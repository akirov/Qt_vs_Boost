You need CMake to build this project. If you have it just do this:

> mkdir build
> cd build
> cmake ../
You may need to specify a generator (build system) in the last command, like this for example:
> cmake -G "Unix Makefiles" ../
or
> cmake -G "MinGW Makefiles" ../

Then build the project:
> make
or
> mingw32-make

The executables will be in build/bin directory.
