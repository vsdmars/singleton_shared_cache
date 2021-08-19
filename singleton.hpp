#pragma once
#include "lrucache.h"
#include "scale-lrucache.h"


using SoftIpCache = LRUC::ScalableLRUCache<int, int>;

extern "C" {
   SoftIpCache* GetSoftIpCache();
}
