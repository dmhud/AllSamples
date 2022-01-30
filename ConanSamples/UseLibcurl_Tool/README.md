#Usage of conan

Added specific tool and generator in `conanfile.txt`:
```
...
[build_requires]
cmake/3.20.0

[generators]
...
VirtualBuildEnv
```

## conan install
```
mkdir build  
cd build  
conan install ..
```

Set/Unset current environment (Path to downloaded package with cmake)
```
cmake --version
conanbuild.bat
cmake --version
...
deactivate_conanbuildenv-release-x86_64.bat
cmake --version
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