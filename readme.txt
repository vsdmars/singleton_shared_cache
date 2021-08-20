# clang++ -std=c++17 -fPIC -c singleton.cpp
# clang++ -std=c++17 -shared singleton.o  -o libsingleton.so

clang++ -std=c++17 -fPIC -c share1.cpp
clang++ -std=c++17 -fPIC -c share2.cpp

clang++ -std=c++17 -shared share1.o -o libshare1.so
clang++ -std=c++17 -shared share2.o -o libshare2.so

clang++ -std=c++17 main.cpp -L. -lshare1 -lshare2 -ltbb

----
Output:
share1: fun address: 0x7fb106fcd5d0
share2: slruc key 1 found
share2: fun address: 0x7fb106fcd5d0
share1: slruc key 2 found
----
