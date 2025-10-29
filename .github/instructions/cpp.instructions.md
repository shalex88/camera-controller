---
applyTo: "**/*.cpp, **/*.h, **/CMakeLists.txt, **/*.cmake"
description: C++ guidelines
---

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

Variables should be declared/defined as const wherever possible

Use constexpr wherever possible

Functions should be marked as const if they don't modify the state of the object

Don't place static functions in header files, place them in anonymous namespaces in cpp files instead

Class members initialization should be defined by default member initializers

Don't write comments in the code, the code should be self-explanatory