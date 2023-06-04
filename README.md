# SChip++

SChip++ is an SChip/Chip8 emulator written in C++. Sound is not implemented.

# Dependencies

This project has the following dependencies:

- `CMake >= 3.8`
- `OpenGL`
- `FreeGLUT`

On windows these can be installed with [vcpkg](https://vcpkg.io).

# Building

This can be done with cmake presets:

```
cmake --preset default
cmake --build --preset default
```

If you are building on windows and have installed the
dependencies with vcpkg, you need to supply cmake with
the vcpkg toolchain file:

```
cmake --preset default --toolchain <vcpkg-root>/scripts/buildsystems/vcpkg.cmake
cmake --build --preset default
```

Substitute \<vcpkg-root\> with wherever you have got vcpkg installed.

# Running

Running the chip8 program is as easy as can be. Simply supply the path
to the Chip8/SChip ROM you want to run as the first argument.
If you run the program without any arguments, it will show you a help text.
