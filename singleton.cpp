#include "singleton.hpp"

namespace {
std::once_flag init_soft_ip_flag;
size_t soft_ip_cache_capacity;
size_t soft_ip_cache_shardCount;
} // namespace

void init_soft_ip_cache(size_t capacity, size_t shardCnt) {
  std::call_once(init_soft_ip_flag, [=] {
    soft_ip_cache_capacity = capacity;
    soft_ip_cache_shardCount = shardCnt;
  });
}

sentinel::SoftIpCache &getSoftIpCache() {
  static sentinel::SoftIpCache cache{soft_ip_cache_capacity,
                                     soft_ip_cache_shardCount};

  return cache;
}
