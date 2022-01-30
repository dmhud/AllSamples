#Instal latest version of conan

Sample from:
https://docs.conan.io/en/latest/creating_packages/getting_started.html

```
$ mkdir hellopkg && cd hellopkg
$ conan new hello/0.1 --template=cmake_lib
File saved: conanfile.py
File saved: CMakeLists.txt
File saved: src/hello.cpp
File saved: src/hello.h
File saved: test_package/conanfile.py
File saved: test_package/CMakeLists.txt
File saved: test_package/src/example.cpp
```
