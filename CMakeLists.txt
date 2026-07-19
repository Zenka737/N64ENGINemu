cmake_minimum_required(VERSION 3.16)
project(N64ENGINemu CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

add_library(n64core STATIC
  src/mem/rom.cpp
  src/mem/rdram.cpp
  src/cpu/vr4300.cpp
)
target_include_directories(n64core PUBLIC include)

add_executable(n64enginemu src/main.cpp)
target_link_libraries(n64enginemu PRIVATE n64core)

option(N64ENGINEMU_BUILD_TESTS "Build unit tests" ON)
if(N64ENGINEMU_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
