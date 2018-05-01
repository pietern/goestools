cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

project(goesdec CXX C)

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^arm")
  # Enable NEON
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=neon")
  set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -mfpu=neon")

  # Assume Raspbian if we're cross compiling for ARM
  if(CMAKE_CROSSCOMPILING)
    # Raspbian supports multiarch and as such places libraries in
    # directories named after their architecture identifier.
    # These paths are not included when cross compiling, so add them here.
    link_directories(${RASPBERRYPI_SYSROOT}/lib/arm-linux-gnueabihf)
    link_directories(${RASPBERRYPI_SYSROOT}/usr/lib/arm-linux-gnueabihf)
  endif()
endif()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/vendor/sanitizers-cmake/cmake ${CMAKE_MODULE_PATH})
find_package(Sanitizers)

# Force static build of libaec
set(BUILD_SHARED_LIBS OFF)

# Set a default build type if none was specified
set(default_build_type "RelWithDebInfo")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Defaulting build type to '${default_build_type}'")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
    STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -pedantic -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -pedantic -Wall")
if("${CMAKE_BUILD_TYPE}" EQUAL "Release")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

add_subdirectory(vendor/libcorrect EXCLUDE_FROM_ALL)
add_subdirectory(vendor/libaec EXCLUDE_FROM_ALL)
add_subdirectory(vendor/json11 EXCLUDE_FROM_ALL)

# Build nanomsg
option(NN_STATIC_LIB "Build nanomsg as static library." ON)
option(NN_ENABLE_DOC "Build nanomsg documentation." OFF)
option(NN_TESTS "Build nanomsg tests" OFF)
option(NN_TOOLS "Build nanomsg tools" OFF)
add_subdirectory(vendor/nanomsg EXCLUDE_FROM_ALL)

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
include_directories(BEFORE SYSTEM ${PROJECT_SOURCE_DIR}/vendor/tinytoml/include)
include_directories(${PROJECT_SOURCE_DIR}/src)

add_subdirectory(src/lib)
add_subdirectory(src/lrit)
add_subdirectory(src/dcs)
add_subdirectory(src/decoder)
add_subdirectory(src/assembler)

option(BUILD_GOESRECV "Build goesrecv" ON)
if(${BUILD_GOESRECV})
  add_subdirectory(src/goesrecv)
endif()

option(BUILD_GOESLRIT "Build goeslrit" ON)
if(${BUILD_GOESLRIT})
  add_subdirectory(src/goeslrit)
endif()

option(BUILD_GOESPROC "Build goesproc" ON)
if(${BUILD_GOESPROC})
  add_subdirectory(src/goesproc)
endif()

option(BUILD_GOESCAT "Build goescat" ON)
if(${BUILD_GOESCAT})
  add_subdirectory(src/goescat)
endif()
