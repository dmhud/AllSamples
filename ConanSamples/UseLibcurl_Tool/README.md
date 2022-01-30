#Usage of conan

## conan install
```
mkdir build  
cd build  
conan install ..
```

Build with cmake:
```
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
```