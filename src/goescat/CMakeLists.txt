add_executable(goescat goescat.cc options.cc)
add_sanitizers(goescat)
install(TARGETS goescat COMPONENT goestools RUNTIME DESTINATION bin)
target_link_libraries(goescat nanomsg packet_reader m stdc++)
