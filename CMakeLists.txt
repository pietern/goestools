cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

project(goesdec CXX C)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/vendor/sanitizers-cmake/cmake ${CMAKE_MODULE_PATH})
find_package(Sanitizers)

# Force static build of libaec
set(BUILD_SHARED_LIBS OFF)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -fPIC -O3 -pedantic -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fPIC -O3 -pedantic -Wall")

add_subdirectory(vendor/libcorrect)
add_subdirectory(vendor/libaec)

# Build nanomsg
option(NN_STATIC_LIB "Build nanomsg as static library." ON)
option(NN_ENABLE_DOC "Build nanomsg documentation." OFF)
option(NN_TESTS "Build nanomsg tests" OFF)
option(NN_TOOLS "Build nanomsg tools" OFF)
add_subdirectory(vendor/nanomsg)

# Symlink vendor/nanomsg/src such that we can still use the "nanomsg"
# include path and not conflict with system includes (e.g. protocol.h).
execute_process(COMMAND
  ${CMAKE_COMMAND} -E make_directory
  ${PROJECT_BINARY_DIR}/include)
execute_process(COMMAND
  ${CMAKE_COMMAND} -E create_symlink
  ${PROJECT_SOURCE_DIR}/vendor/nanomsg/src
  ${PROJECT_BINARY_DIR}/include/nanomsg)

include_directories(BEFORE SYSTEM ${PROJECT_BINARY_DIR}/include)
include_directories(BEFORE SYSTEM ${PROJECT_SOURCE_DIR}/vendor/libcorrect/include)
include_directories(BEFORE SYSTEM ${PROJECT_SOURCE_DIR}/vendor/libaec/src)
include_directories(BEFORE SYSTEM ${PROJECT_SOURCE_DIR}/vendor/libsimdpp)
include_directories(BEFORE SYSTEM ${PROJECT_SOURCE_DIR}/vendor/tinytoml/include)
include_directories(${PROJECT_SOURCE_DIR}/src)

option(BUILD_GOESRECV "Build goesrecv" OFF)
option(BUILD_GOESDEC "Build goesdec" ON)
option(BUILD_GOESPROC "Build goesproc" ON)

add_subdirectory(src/lib)
add_subdirectory(src/lrit)
add_subdirectory(src/dcs)
add_subdirectory(src/decoder)
add_subdirectory(src/assembler)

# Only compile goesrecv if target is Raspberry Pi.
# It includes unguarded ARM NEON code. Assume the variable that is
# tested below is set in the Raspberry Pi specific toolchain.cmake.
# Configure CMake with -DBUILD_GOESRECV=ON to compile.
if(${BUILD_GOESRECV})
  add_subdirectory(src/goesrecv)
endif()

if(${BUILD_GOESDEC})
  add_subdirectory(src/goesdec)
endif()

if(${BUILD_GOESPROC})
  add_subdirectory(src/goesproc)
endif()
