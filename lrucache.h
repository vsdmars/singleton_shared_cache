/**
 * @author shchang
 */

#pragma once

#include <atomic>
#include <mutex>
#include <new>
#include <thread>
#include <vector>
#include <tbb/concurrent_hash_map.h>

namespace LRUC {

/**
 * LRUCache is a hash-table data structure provides thread-safe access with
 * defined size limit.
 *
 * As a Least Recently Used (LRU) Cache, when the cache is full(hit the upper
 * bound of the defined capacity), insert() evicts the least recently used key from
 * the cache.
 *
 * find() takes ConstAccessor as carrier for referring to found value inside the LRUCache.
 * The found value is deleted from memory iff ConstAccessor is destructed.
 * Updating the frequency for find could fail due to by contract find() should not stall.
 *
 * To avoid increasing latency of concurrent insert() call due to concurrent_hash_map
 * lock shall we use sharded key with multiple LRUCache instance instead.
 *
 * Internal double-linked list is guarded with mutex for modifying the list.
 *
 * Type concepts:
 *  Types TKey and TValue must model the CopyConstructible concept.
 *  For previous intel TBB, the TValue should also have DefaultConstructible concept due
 *  to TBB will use TValue() as placeholder for key finding.
 *  As for latest intel TBB has no such issue.
 *
 *  Type THash must model the TBB::HashCompare concept.
 *  Good performance depends on having good pseudo-randomness in the low-order bits of the hash code.
 *  When keys are pointers, simply casting the pointer to a hash code may cause poor performance because the low-order
 *   bits of the hash code will be always zero if the pointer points to a type with alignment restrictions. A way to
 *   remove this bias is to divide the casted pointer by the size of the type.
 *
 * Exception Safety:
 * The following functions must not throw exceptions:
 *  The hash function
 *  The destructors for types TKey and TValue.
 *
 * The following holds true:
 *  If an exception happens during an insert operation, the operation has no effect.
 *  If an exception happens during an assignment operation, the container may be in a state where only some of the items
 *    were assigned, and methods size() and empty() may return invalid answers.
 *
 * Reference:
 * https://spec.oneapi.com/versions/latest/elements/oneTBB/source/containers/concurrent_hash_map_cls.html
 *
 * LRUCache is C++17 compatible
 */

template <typename TKey, typename TValue, typename THash = tbb::tbb_hash_compare<TKey>>
class LRUCache final {
 private:
  struct Value;
  using HashMap = tbb::concurrent_hash_map<TKey, Value, THash>;
  using HashMapConstAccessor = typename HashMap::const_accessor;
  using HashMapAccessor = typename HashMap::accessor;
  using HashMapValuePair = typename HashMap::value_type;
  using ListMutex = std::mutex;

 private:
  struct ListNode;
  // used for judging a node exist inside the double-linked list.
  static ListNode* const NullNodePtr;

 private:
  /**
   * ListNode is the element type forms the internal double-linked list,
   * which serves as the LRU cache eviction manipulator.
   */
  struct ListNode final {
    TKey key_;
    ListNode* prev_;
    ListNode* next_;

    constexpr ListNode() : prev_(NullNodePtr), next_(nullptr) {}

    // Avoid unintended conversions.
    // https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-explicit
    explicit constexpr ListNode(const TKey& key) : key_(key), prev_(NullNodePtr), next_(nullptr) {}

    // return false if node is not in cache's double-linked list.
    constexpr bool inList() const {
      return prev_ != NullNodePtr;
    }
  };

  /**
   * Value is the value stored in the hash-table.
   * listNode_ data member as back-reference to node to the double-linked list,
   * which contains hash-table key.
   */
  struct Value final {
    // could use std::ref/std::cref for uncopyable type
    TValue value_;
    ListNode* listNode_;

    constexpr Value() = default;
    constexpr Value(const TValue& value, ListNode* node) : value_(value), listNode_(node) {}
  };

 private:
  /**
   * intel TBB concurrent_hash_map
   */
  HashMap hash_map_;

  /**
   * Current LRUCache size.
   */
  std::atomic<size_t> current_size_;

  /**
   * head_ is the least-recently used node.
   * tail_ is the most-recently used node.
   * listMutex should be held during list modification.
   */
  ListNode head_;
  ListNode tail_;
  ListMutex listMutex_;

  /**
   * LRUCache size.
   */
  size_t cache_size_;

 private:
  /**
   * Append a node to the double-linked list as the most-recently used.
   * Not thread-safe. Caller is responsible for a lock.
   */
  void append(ListNode* node);

  /**
   * Unlink a node from the list.
   * Not thread-safe. Caller is responsible for a lock.
   */
  void unlink(ListNode* node);

  /**
   * Remove the least-recently used value from the LRUCache.
   * Thread-safe.
   */
  void popFront();

 public:
  /**
   * Helper type wraped over tbb::concurrent_hash_map::const_accessor with
   * operator overloaded to retrieve value stored in the hash-table based on
   * key.
   */
  struct ConstAccessor final {
    constexpr ConstAccessor() = default;

    constexpr const TValue& operator*() const {
      return *get();
    }

    constexpr const TValue* operator->() const {
      return get();
    }

    constexpr bool empty() const {
      return hashAccessor_.empty();
    }

    constexpr const TValue* get() const {
      return &value_;
    }

    constexpr void release() {
      hashAccessor_.release();
    }

   private:
    /**
     * copy TValue from concurrent_hash_map thus caller could release lock early.
     */
    void setValue() {
      value_ = hashAccessor_->second.value_;
    }

   private:
    friend class LRUCache;  // for LRUCache member function to access tbb::concurrent_hash_map::const_accessor
    HashMapConstAccessor hashAccessor_;
    TValue value_;
  };

  /**
   * size as the initial size for LRUCache.
   * The size should be tunable at run-time (Phase II)
   *
   * bucketCount is used for initial setup the tbb:concurrent_hash_map, the bucket size
   * will grow depends on internal algorithm.
   */
  explicit LRUCache(size_t size, size_t bucketCount = std::thread::hardware_concurrency() * 4);

  ~LRUCache() {
    clear();
  }

  LRUCache(const LRUCache& other) = delete;
  LRUCache& operator=(const LRUCache&) = delete;

  /**
   * Erase removes key from LRUCache along with its value.
   * returns number of elements removed (0 or 1).
   */
  size_t erase(const TKey& key);

  /**
   * Find data inside hash-table through provided key.
   * ConstAccessor stores the found result.
   * Return true is key exist, otherwise return false.
   *
   * Find updates key access frequency.
   * Update access frequency could fail.
   */
  bool find(ConstAccessor& ac, const TKey& key);

  /**
   * Insert key/value into LRUCache. Both key and value is copied into the cache.
   * Insert updates key access frequency.
   *
   * If key already exists in the LRUCache, the value will not be updated and return
   * false. Otherwise return true.
   */
  bool insert(const TKey& key, const TValue& value);

  /**
   * Erases all elements from the container.
   * After this call, size() returns zero.
   * Not thread-safe.
   */
  void clear();

  /**
   * Returns the number of elements in the container.
   */
  size_t size() const {
    return current_size_.load();
  }

  /**
   * Returns LRUCache capacity
   */
  constexpr size_t capacity() const {
    return cache_size_;
  }
};

template <class TKey, class TValue, class THash>
typename LRUCache<TKey, TValue, THash>::ListNode* const LRUCache<TKey, TValue, THash>::NullNodePtr = (ListNode*) - 1;

// ---- private member functions ----
template <class TKey, class TValue, class THash>
inline void LRUCache<TKey, TValue, THash>::unlink(ListNode* node) {
  ListNode* prev = node->prev_;
  ListNode* next = node->next_;
  prev->next_ = next;
  next->prev_ = prev;
  // assign to NullNodePtr as indicator for this node is no longer in the double-linked list.
  node->prev_ = NullNodePtr;
}

template <class TKey, class TValue, class THash>
inline void LRUCache<TKey, TValue, THash>::append(ListNode* node) {
  ListNode* prevLatestNode = tail_.prev_;

  node->next_ = &tail_;
  node->prev_ = prevLatestNode;

  tail_.prev_ = node;
  prevLatestNode->next_ = node;
}

template <class TKey, class TValue, class THash>
void LRUCache<TKey, TValue, THash>::popFront() {
  ListNode* candidate = nullptr;
  TKey tmpKey;

  {
    std::unique_lock<ListMutex> lock(listMutex_);

    candidate = head_.next_;
    // empty double-linked list check
    if (candidate == &tail_) {
      return;
    }

    unlink(candidate);

    tmpKey = candidate->key_;

    delete candidate;
  }

  HashMapConstAccessor constHashAccessor;
  if (!hash_map_.find(constHashAccessor, tmpKey)) {
    return;
  }

  hash_map_.erase(constHashAccessor);

}

// ---- private member functions end ----

template <class TKey, class TValue, class THash>
LRUCache<TKey, TValue, THash>::LRUCache(size_t size, size_t bucketCount)
  : hash_map_(bucketCount), current_size_(0), cache_size_(size) {
  head_.prev_ = nullptr;
  head_.next_ = &tail_;
  tail_.prev_ = &head_;
}

template <class TKey, class TValue, class THash>
size_t LRUCache<TKey, TValue, THash>::erase(const TKey& key) {
  ListNode* found_node;

  // immutable read accessor
  HashMapConstAccessor hashAccessor{};
  if (!hash_map_.find(hashAccessor, key)) {
    hashAccessor.release();  // release early
    return 0;
  } else {
    found_node = hashAccessor->second.listNode_;
    hashAccessor.release();  // release early
  }

  // tbb::concurrent_hash_map.erase(key) by contract won't throw exception if key does not exist.
  hash_map_.erase(key);

  {
    // Update double-linked list before update current_size_
    std::unique_lock<ListMutex> lock(listMutex_);
    if (found_node->inList()) {
      unlink(found_node);
      delete found_node;
    }
  }


  current_size_--;

  return 1;
}

template <class TKey, class TValue, class THash>
bool LRUCache<TKey, TValue, THash>::find(ConstAccessor& caccessor, const TKey& key) {
  ListNode* found_node;

  // immutable read accessor
  HashMapConstAccessor& hashAccessor = caccessor.hashAccessor_;
  if (!hash_map_.find(hashAccessor, key)) {
    hashAccessor.release();  // release early
    return false;
  } else {
    caccessor.setValue();
    found_node = hashAccessor->second.listNode_;
    hashAccessor.release();  // release early
  }

  {
    // Key found, update double-linked list with try lock.
    std::unique_lock<ListMutex> lock{listMutex_, std::try_to_lock};
    if (lock) {
      if (found_node->inList()) {
        unlink(found_node);
        append(found_node);
      }
    }
  }

  return true;
}

template <class TKey, class TValue, class THash>
bool LRUCache<TKey, TValue, THash>::insert(const TKey& key, const TValue& value) {
  // create node with key through default new allocator.
  ListNode* node = new ListNode(key);

  {
    // release HashMapAccessor early
    HashMapAccessor hashAccessor;
    HashMapValuePair hashMapValue(key, Value(value, node));
    // hashMapValue is copied and memory allocated in concurrent_hash_map
    if (!hash_map_.insert(hashAccessor, hashMapValue)) {
      delete node;
      return false;
    }
  }

  // While hits LRUCache capacity, evict one item from double-linked list.
  // Happens before insertion.
  size_t size = current_size_.load();
  bool popped = false;
  if (size >= cache_size_) {
    popFront();
    popped = true;
  }

  {
    // Update double-linked list before update current_size_
    std::unique_lock<ListMutex> lock(listMutex_);
    append(node);
  }

  // only update atomic if there's no eviction.
  if (!popped) {
    size = current_size_++;
  }

  // Check size before this insertion while there could be other threads
  // updating the cache and had the cache exceed the defined cache size.
  // Evict one node per insertion, avoid while loop with compare_exchange_weak
  // which consumes power and increases latency for insertion.
  if (size > cache_size_) {
    // Use compare_exchange_strong with default sequential consistency memory model.
    // Update double-linked list iff there's no value change in between
    // previous load expression to (size - 1).
    if (current_size_.compare_exchange_strong(size, size - 1)) {
      popFront();
    }
  }

  return true;
}

template <class TKey, class TValue, class THash>
void LRUCache<TKey, TValue, THash>::clear() {
  hash_map_.clear();

  ListNode* node = head_.next_;

  ListNode* next;
  while (node != &tail_) {
    next = node->next_;
    delete node;
    node = next;
  }

  head_.next_ = &tail_;
  tail_.prev_ = &head_;
  current_size_ = 0;
}
}  // namespace LRUC
