#Instal latest version of conan

Install latest version:
```
pip install conan
``` 

Uninstal older version if needed:
```
pip uninstall conan
``` 

Upgrade conan
```
pip install conan --upgrade  # Might need sudo or --user
``` 

Default profile in file: `c:\Users\<USER>\.conan\profiles\default`
```
[settings]
os=Windows
os_build=Windows
arch=x86_64
arch_build=x86_64
compiler=msvc
compiler.version=193
compiler.cppstd=20
compiler.runtime=static
build_type=Release
[options]
[build_requires]
[env]
```