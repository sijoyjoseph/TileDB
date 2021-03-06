#
# CMakeLists.txt
#
#
# The MIT License
#
# Copyright (c) 2017-2018 TileDB, Inc.
# Copyright (c) 2016 MIT and Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

############################################################
# CMake setup
############################################################

cmake_minimum_required(VERSION 3.3)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(TILEDB_CMAKE_INPUTS_DIR "${CMAKE_SOURCE_DIR}/cmake/inputs")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Only for Mac OS X warnings
if(APPLE)
  set(CMAKE_MACOSX_RPATH 1)
endif()

# Set C++11 as required standard for all C++ targets.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Don't use GNU extensions

# Set -fvisibility=hidden (or equivalent) flags by default.
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

############################################################
# Build options
############################################################

# Note: when adding options, make sure to forward them via INHERITED_CMAKE_ARGS
# in TileDB-Superbuild.cmake.

option(TILEDB_SUPERBUILD "If true, perform a superbuild (builds all missing dependencies)." ON)
option(TILEDB_FORCE_ALL_DEPS "If true, force superbuild to download and build all dependencies, even those installed on the system." OFF)
option(TILEDB_VERBOSE "Prints TileDB errors with verbosity" OFF)
option(TILEDB_S3 "Enables S3/minio support using aws-cpp-sdk" OFF)
option(TILEDB_HDFS "Enables HDFS support using the official Hadoop JNI bindings" OFF)
option(TILEDB_WERROR "Enables the -Werror flag during compilation." ON)
option(TILEDB_CPP_API "Enables building of the TileDB C++ API." ON)
option(TILEDB_CMAKE_IDE "(Used for CLion builds). Disables superbuild and sets the EP install dir." OFF)

############################################################
# Superbuild setup
############################################################

# Set the variable used when calling find_package(), find_file() etc.
# to determine if NO_DEFAULT_PATH should be passed.
if (TILEDB_FORCE_ALL_DEPS)
  set(TILEDB_DEPS_NO_DEFAULT_PATH NO_DEFAULT_PATH)
else()
  set(TILEDB_DEPS_NO_DEFAULT_PATH)
endif()

# If this is an in-IDE build, we need to disable the superbuild and explicitly
# set the EP base dir. The normal 'cmake && make' process won't need this step,
# it is for better CLion support of this superbuild architecture.
if (TILEDB_CMAKE_IDE)
  set(TILEDB_SUPERBUILD OFF)
  set(TILEDB_EP_BASE "${CMAKE_BINARY_DIR}/externals")
endif()

# Perform superbuild config and exit.
if (TILEDB_SUPERBUILD)
  project(TileDB-Superbuild)
  message(STATUS "Starting TileDB superbuild.")
  include("cmake/TileDB-Superbuild.cmake")
  message(STATUS "Install prefix is ${CMAKE_INSTALL_PREFIX}")
  # Stop processing this file here.
  return()
endif()

project(TileDB)
message(STATUS "Starting TileDB regular build.")
# Paths to locate the installed external projects.
set(TILEDB_EP_SOURCE_DIR "${TILEDB_EP_BASE}/src")
set(TILEDB_EP_INSTALL_PREFIX "${TILEDB_EP_BASE}/install")

############################################################
# Compile options/definitions for all targets
############################################################

# Set compiler flags
if (WIN32)
  # We disable some warnings that are not present in gcc/clang -Wall:
  #   C4101: unreferenced local variable
  #   C4244: conversion warning of floating point to integer type.
  #   C4251: C++ export warning
  #   C4456: local variable hiding previous local variable
  #   C4457: local variable hiding function parameter
  #   C4702: unreachable code
  #   C4800: warning implicit cast int to bool
  #   C4996: deprecation warning about e.g. sscanf.
  add_compile_options(/W4 /wd4101 /wd4244 /wd4251 /wd4456 /wd4457 /wd4702 /wd4800 /wd4996)
  # Warnings as errors:
  if (TILEDB_WERROR)
    add_compile_options(/WX)
  endif()
  # Disable GDI (which we don't need, and causes some macro
  # re-definition issues if wingdi.h is included)
  add_compile_options(/DNOGDI)
  # Add /MPn flag from CMake invocation (if defined).
  add_compile_options(${MSVC_MP_FLAG})
  # Build-specific flags
  if (CMAKE_BUILD_TYPE MATCHES "Debug")
    add_compile_options(/DDEBUG /Od /Zi)
  elseif (CMAKE_BUILD_TYPE MATCHES "Release")
    add_compile_options(/DNDEBUG /Ox)
  endif()
else()
  add_compile_options(-Wall -Wextra)
  if (TILEDB_WERROR)
    add_compile_options(-Werror)
  endif()
  # Build-specific flags
  if (CMAKE_BUILD_TYPE MATCHES "Debug")
    add_compile_options(-DDEBUG -O0 -g3 -ggdb3 -gdwarf-3)
  elseif (CMAKE_BUILD_TYPE MATCHES "Release")
    add_compile_options(-DNDEBUG -O3)
  elseif (CMAKE_BUILD_TYPE MATCHES "Coverage")
    add_compile_options(-DDEBUG -g3 -gdwarf-3 --coverage)
  endif()

  # Use -Wno-literal-suffix on Linux with C++ sources.
  if (NOT APPLE)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-literal-suffix>)
  endif()
endif()

# Definitions for all targets
add_definitions(-D_FILE_OFFSET_BITS=64)

############################################################
# Enable testing and add subdirectories
############################################################

# Enable testing
enable_testing()

# Build the TileDB library
add_subdirectory(tiledb)

# Build examples
add_subdirectory(examples)

# Build unit tests
add_subdirectory(test)

###########################################################
# Uninstall
###########################################################

set(CMD "xargs printf -- '-- Uninstalling: %s\\\\n' <install_manifest.txt")
add_custom_target(
   uninstall
   COMMAND echo "Uninstalling TileDB from ${CMAKE_INSTALL_PREFIX}..."
   COMMAND eval "${CMD}"
   COMMAND xargs rm -f < install_manifest.txt
   COMMAND rmdir "${CMAKE_INSTALL_PREFIX}/include/tiledb"
   COMMAND echo "TileDB uninstalled"
)
