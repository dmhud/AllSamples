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
cmake --build . --config=Release
```

Run application:
```
Release\mycurl.exe https://catfact.ninja/fact
Release\mycurl.exe https://yandex.ru
Release\mycurl.exe https://www.google.com
```