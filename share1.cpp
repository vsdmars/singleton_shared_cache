#include "singleton.hpp"
#include <iostream>

using namespace std;

void run1() {
  init_soft_ip_cache(7, 4);
  auto &slruc = getSoftIpCache();
  cout << "share1: fun address: " << &slruc << endl;
  cout << "share1 capacity: " << slruc.capacity() << endl;

  // test ScalableLRUCache
  slruc.insert(1, 11);
}

void check1() {
  auto &slruc = getSoftIpCache();

  sentinel::SoftIpCache::ConstAccessor sca;
  if (!slruc.find(sca, 2)) {
    std::cout << "share1: slruc key 2 not found\n" << std::flush;
  } else {
    std::cout << "share1: slruc key 2 found\n" << std::flush;
  }
}
