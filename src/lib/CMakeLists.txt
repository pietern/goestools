add_library(util util.cc)
add_library(packet_reader packet_reader.cc nanomsg_reader.cc file_reader.cc)
target_link_libraries(packet_reader nanomsg)
