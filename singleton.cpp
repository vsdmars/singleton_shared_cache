#include "singleton.hpp"

sentinel::SoftIpCache *GetSoftIpCache(size_t capacity, size_t shardCount) {
  static auto cache = [=]() {
    static sentinel::SoftIpCache cache{capacity, shardCount};

    return &cache;
  };

  return cache();
}

void *GetSoftIpCache() {
  static sentinel::SoftIpCache cache{7, 4};

  return &cache;
}
