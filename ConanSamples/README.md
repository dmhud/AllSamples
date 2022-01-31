# Instal latest version of conan

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
compiler=Visual Studio
compiler.runtime=MD
compiler.version=16
build_type=Release
[options]
[build_requires]
[env]
```

Create dependency graph in HTML format and open in browser
```
conan info .. --graph=graph.html
graph.html
```

Create package with default profile
```
conan create .
```

Create package with other wettings
```
conan create . -s arch=x86 -s compiler.version=15 -s compiler.runtime=MT
conan create . -s compiler.version=17 -s compiler.runtime=MT
```

Search packages in local cache (Last example with symbol @ - show info of all configurations/settings in package
```
conan search
conan search hello/0.1
conan search hello/0.1@
```
