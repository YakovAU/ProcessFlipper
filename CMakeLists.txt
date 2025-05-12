cmake_minimum_required(VERSION 3.29)
project(ProcessFlipper)

# Enable the Resource Compiler language
enable_language(RC)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find and setup ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# Check for LTO support
include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT error_message)

# Set optimization flags for Release builds
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Strip symbols in Release mode
if(WIN32)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
else()
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
endif()

# Default to Release build if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(ProcessFlipper WIN32 main.cpp
        process_manager.cpp
        tray_icon.cpp
        app.rc
        # List header files for clarity / IDE support
        tray_icon.h
        hotkey_manager.h
        process_manager.h
        resource.h
)

# Enable LTO/IPO if supported
if(lto_supported)
  set_property(TARGET ProcessFlipper PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
  message(WARNING "LTO is not supported: ${error_message}")
endif()

# Use precompiled headers for common headers
target_precompile_headers(ProcessFlipper PRIVATE
  <vector>
  <string>
  <windows.h>
  <memory>
  <unordered_map>
)

# Optimize for the current CPU architecture if not on Windows
# (Windows builds typically need to be more portable, so architecture-specific optimizations are often avoided)
if(NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()

# For Windows builds, specific optimizations
if(WIN32)
  # Use whole program optimization for MSVC
  target_compile_options(ProcessFlipper PRIVATE $<$<CONFIG:Release>:/GL>)
  # Use LTCG (Link Time Code Generation) for the linker
  set_property(TARGET ProcessFlipper PROPERTY LINK_FLAGS_RELEASE "/LTCG")
endif()