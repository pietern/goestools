add_executable(goeslrit goeslrit.cc options.cc reader.cc file_reader.cc nanomsg_reader.cc)
add_sanitizers(goeslrit)
target_link_libraries(goeslrit assembler nanomsg stdc++)
