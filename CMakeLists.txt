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

# Windows video is native Win32/GDI (no extra dependency to install);
# everywhere else it's backed by SDL2.
if(NOT WIN32)
  find_package(SDL2 REQUIRED)
endif()

add_library(n64core STATIC
  src/mem/rom.cpp
  src/mem/rdram.cpp
  src/cpu/vr4300.cpp
  src/video/frame_timing.cpp
  src/video/vi.cpp
  src/mem/controller.cpp
  src/video/sprite_scene.cpp
)
target_include_directories(n64core PUBLIC include)

# The windowing layer is kept separate from n64core so the unit tests (which
# link n64core) never need a display, SDL, or Win32 GUI APIs at all.
add_executable(n64enginemu
  src/main.cpp
  src/video/video.cpp
  src/input/input.cpp
)
target_link_libraries(n64enginemu PRIVATE n64core)
if(WIN32)
  target_link_libraries(n64enginemu PRIVATE comdlg32 gdi32)
else()
  target_link_libraries(n64enginemu PRIVATE SDL2::SDL2)
endif()

option(N64ENGINEMU_BUILD_TESTS "Build unit tests" ON)
if(N64ENGINEMU_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
