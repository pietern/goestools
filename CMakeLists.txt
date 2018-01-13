cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

project(goesdec CXX C)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/vendor/sanitizers-cmake/cmake ${CMAKE_MODULE_PATH})
find_package(Sanitizers)

# Force static build of libaec
set(BUILD_SHARED_LIBS OFF)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -fPIC -pedantic -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fPIC -pedantic -Wall")

add_subdirectory(vendor/libcorrect)
add_subdirectory(vendor/libaec)

include_directories(BEFORE SYSTEM ${PROJECT_SOURCE_DIR}/vendor/libcorrect/include)
include_directories(BEFORE SYSTEM ${PROJECT_SOURCE_DIR}/vendor/libaec/src)
include_directories(BEFORE SYSTEM ${PROJECT_SOURCE_DIR}/vendor/libsimdpp)
include_directories(${PROJECT_SOURCE_DIR}/src)

add_subdirectory(src/lib)
add_subdirectory(src/goesdec)
add_subdirectory(src/goesproc)
