#include "singleton.hpp"
#include <iostream>

using namespace std;

void run2() {
  auto slruc = GetSoftIpCache<>();
  cout << "share2: fun address: " << slruc << endl;

  // test ScalableLRUCache
  slruc->insert(2, 22);
}

void check2() {
  auto slruc = GetSoftIpCache<>();

  sentinel::SoftIpCache::ConstAccessor sca;
  if (!slruc->find(sca, 1)) {
    std::cout << "share2: slruc key 1 not found\n" << std::flush;
  } else {
    std::cout << "share2: slruc key 1 found\n" << std::flush;
  }
}
