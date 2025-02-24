# Physics Sandbox
A fun project to prototype 3D physics simulations. <br>
Initially using OpenGL as the graphics API but will likely upgrade in the future if I work on this long enough / actually properly learn vulkan

## Compilation
To compile all you should need is CMake.<br>
Simply make a directory called build while at the root of this repo and run the cmake commands<br>

For example to build this project on windows using MinGW all you need to do is:
```
mkdir build
cd build
cmake -G "MinGW Makefiles"
mingw32-make
```

## Structure
This project will mostly be structured like a it was in C. C++ is mainly being used for its data structures and so I can use certain libraries (i.e. assimp). There might be a few classes here and there and some smart pointers but will mostly try to stick to basic functions and structs.<br>
The reason for this is simply because I like that style of programming and this is meant to be fun as well as educational...