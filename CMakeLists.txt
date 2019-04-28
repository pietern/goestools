cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

project(goesdec CXX C)

# Remove directory from CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES and add it
# to the link directories list. This is needed to prioritize the
# libraries in that directory over the ones stored in the implicit
# link directories. This is done to make a cross compiler find and use
# a different default library (e.g. libc or stdc++) from the one it
# ships with. For example, Raspbian uses a different libc (version
# 2.24) than the one that comes with GCC 5 for arm-linux-gnueabihf in
# Ubuntu 18.04 (version 2.27), and mixing the newer one with Raspbian
# system libraries causes symbol resolution errors.
macro(shift_link_directories dir)
  list(REMOVE_ITEM CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "${dir}")
  link_directories("${dir}")
endmacro()

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^arm")
  # Enable NEON
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=neon")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon")

  # Assume Raspbian if we're cross compiling for ARM
  if(CMAKE_CROSSCOMPILING)
    # Raspbian supports multiarch and as such places libraries in
    # directories named after their architecture identifier.
    shift_link_directories(${RASPBERRYPI_SYSROOT}/lib/arm-linux-gnueabihf)
    shift_link_directories(${RASPBERRYPI_SYSROOT}/usr/lib/arm-linux-gnueabihf)
    set(ENV{PKG_CONFIG_PATH} "${RASPBERRYPI_SYSROOT}/usr/lib/arm-linux-gnueabihf/pkgconfig")
    set(ENV{PKG_CONFIG_SYSROOT_DIR} "${RASPBERRYPI_SYSROOT}")
  endif()
endif()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/vendor/sanitizers-cmake/cmake ${CMAKE_MODULE_PATH})
find_package(Sanitizers)

# Force static build of libaec
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)

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
add_subdirectory(vendor/json EXCLUDE_FROM_ALL)

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

add_subdirectory(src/util)
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

# Disable goesemwin by default because it won't be relevant once
# GOES-15 is replaced by GOES-17 in late 2018.
option(BUILD_GOESEMWIN "Build goesemwin" OFF)
if(${BUILD_GOESEMWIN})
  add_subdirectory(src/goesemwin)
endif()

option(BUILD_GOESPROC "Build goesproc" ON)
if(${BUILD_GOESPROC})
  add_subdirectory(src/goesproc)

  # Process and install GOES-N series example config
  configure_file(
    share/goesproc-goesn.conf.in
    "${PROJECT_BINARY_DIR}/share/goesproc-goesn.conf")
  install(
    FILES "${PROJECT_BINARY_DIR}/share/goesproc-goesn.conf"
    DESTINATION share/goestools
    COMPONENT goestools)

  # Process and install GOES-R series example config
  configure_file(
    share/goesproc-goesr.conf.in
    "${PROJECT_BINARY_DIR}/share/goesproc-goesr.conf")
  install(
    FILES "${PROJECT_BINARY_DIR}/share/goesproc-goesr.conf"
    DESTINATION share/goestools
    COMPONENT goestools)

  # Install contrast curve and false color LUT
  install(
    DIRECTORY share/wxstar
    DESTINATION share/goestools
    COMPONENT goestools)

  # Install Natural Earth GeoJSON files for map overlays
  install(
    DIRECTORY share/ne
    DESTINATION share/goestools
    COMPONENT goestools)
endif()

option(BUILD_GOESPACKETS "Build goespackets" ON)
if(${BUILD_GOESPACKETS})
  add_subdirectory(src/goespackets)
endif()
