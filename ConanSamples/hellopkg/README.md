# Instal latest version of conan

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


Build and run with default profile:
```
conan create . demo/testing
```

Build and run with default profile, but ovveride build to Debug configuration:
```
conan create . demo/testing -s build_type=Debug
```

Build and run with default profile, but override library type to dynamic (DLL):
```
conan create . demo/testing -o hello:shared=True
```


Possible Output:
```
...
hello/0.1: Hello World Release!
  hello/0.1: _M_X64 defined
  hello/0.1: _MSC_VER1930
  hello/0.1: _MSVC_LANG202002
  hello/0.1: __cplusplus199711
  ...
```
