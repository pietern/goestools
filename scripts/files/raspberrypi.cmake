# Define our target system
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)
SET(CMAKE_SYSTEM_PROCESSOR armv7l)

# Assume this file is located next to the "tools" repository
SET(RASPBERRYPI_ROOT ${CMAKE_CURRENT_LIST_DIR})
SET(RASPBERRYPI_SYSROOT ${RASPBERRYPI_ROOT}/sysroot)

# Define the cross compiler locations
SET(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc-5)
SET(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++-5)

# Define the sysroot path for CMake
SET(CMAKE_SYSROOT ${RASPBERRYPI_SYSROOT})
SET(CMAKE_FIND_ROOT_PATH ${RASPBERRYPI_SYSROOT})

# Use our definitions for compiler tools
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories only
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
