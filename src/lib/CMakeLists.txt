add_library(dir dir.cc)
add_library(zip zip.cc)
add_library(timer timer.cc)
target_link_libraries(zip z)
add_library(packet_reader packet_reader.cc nanomsg_reader.cc file_reader.cc)
target_link_libraries(packet_reader nanomsg)
add_library(packet_writer packet_writer.cc nanomsg_writer.cc file_writer.cc)
target_link_libraries(packet_writer nanomsg)
add_executable(unzip unzip.cc)
target_link_libraries(unzip zip m stdc++)

# Get the latest abbreviated commit hash of the working branch
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )

# Get the latest commit date of the working branch
execute_process(
  COMMAND git log -1 --format=%cD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_DATE
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )

# Generate header file with string macros
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/version-gen.h.in
  ${CMAKE_BINARY_DIR}/include/version-gen.h
  )

add_library(version version.cc)
target_include_directories(version PRIVATE ${CMAKE_BINARY_DIR}/include)
