#include "singleton.hpp"

void *GetSoftIpCache() {
  static sentinel::SoftIpCache cache{7, 4};

  return &cache;
}
