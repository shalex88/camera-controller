---
applyTo: "**/*.cpp, **/*.h, **/CMakeLists.txt, **/*.cmake"
description: C++ guidelines
---

Add a header comment to C++ files: 'Follows C++ guidelines'

Use C++20

Support only GCC compiler

Use CMake as a build, test, packaging and installation system

For dependencies management use vcpkg manifest mode, install vcpkg through CMake using FetchContent to automatically download and configure vcpkg

For testing use GoogleTest

Use spdlog for logging

Use ```#pragma once``` in header files

Use camelCase for function and methods names

Use snake_case for variable names

Use _ suffix for private member variables

Use PascalCase for class names

Use Stroustrup style for C++ code

Don't use raw pointers, use smart pointers instead

Use Result<T, E> for error handling

Variables should always be initialized

Variables should be declared as const if possible

No need to write comments in the code, the code should be self-explanatory