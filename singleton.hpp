#pragma once

#include "lrucache.h"
#include "scale-lrucache.h"

namespace sentinel {

enum class CACHE_VALUE_TYPE {
  TIME_ENTITY_LOOKUP_INFO,
};

template <CACHE_VALUE_TYPE E = CACHE_VALUE_TYPE::TIME_ENTITY_LOOKUP_INFO>
struct CacheValue {
  int64_t expiryTs;
  int denialInfoCode;
  int routingPrefixSize;
  bool requiresGoodBotUserAgent;

  CacheValue() = default;
  CacheValue(int64_t ts, int infoCode = 0, int prefixSize = 32,
             bool requiresGoodUserAgent = false)
      : expiryTs(ts), denialInfoCode(infoCode), routingPrefixSize(prefixSize),
        requiresGoodBotUserAgent(requiresGoodUserAgent){};
};

using SoftIpCache = LRUC::ScalableLRUCache<int, CacheValue<>>;

} // namespace sentinel

template <int capacity, int shardedCount> sentinel::SoftIpCache *GetSoftIpCache() {
    // make configurable argument from the value type template argument to avoid Static Initialization Order Fiasco.
    static sentinel::SoftIpCache cache{capacity, shardedCount};

    return &cache;
}

extern "C" {
void *GetSoftIpCache();
}
