add_executable(goeslrit goeslrit.cc options.cc)
add_sanitizers(goeslrit)
target_link_libraries(goeslrit assembler nanomsg packet_reader stdc++)
