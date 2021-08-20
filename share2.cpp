#include "singleton.hpp"
#include <iostream>

using namespace std;

void run2() {
  init_soft_ip_cache(7, 4);
  auto &slruc = getSoftIpCache();
  cout << "share2: fun address: " << &slruc << endl;
  cout << "share2 capacity: " << slruc.capacity() << endl;

  // test ScalableLRUCache
  slruc.insert(2, 22);
}

void check2() {
  auto &slruc = getSoftIpCache();

  sentinel::SoftIpCache::ConstAccessor sca;
  if (!slruc.find(sca, 1)) {
    std::cout << "share2: slruc key 1 not found\n" << std::flush;
  } else {
    std::cout << "share2: slruc key 1 found\n" << std::flush;
  }
}
