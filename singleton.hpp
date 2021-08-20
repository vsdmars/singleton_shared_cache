#pragma once

#include "lrucache.h"
#include "scale-lrucache.h"
#include <mutex>

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

void init_soft_ip_cache(size_t capacity, size_t shardCnt);
sentinel::SoftIpCache &getSoftIpCache();
