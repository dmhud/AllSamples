#Usage of conan

## conan install
```
mkdir build  
cd build  
conan install ..
```
`conan install` will do a few things:
- fetch more than one library. In `conanfile.txt` specified only `libcurl`. Conan fetch transit dependencies of libcurl 

Possible output of `conan install`
```

```