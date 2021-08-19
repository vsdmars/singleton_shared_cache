#include "singleton.hpp"

SoftIpCache* GetSoftIpCache() {
    static SoftIpCache cache{7, 4};

    return &cache;
}
