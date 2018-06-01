add_executable(goesemwin goesemwin.cc options.cc)
add_sanitizers(goesemwin)
install(TARGETS goesemwin COMPONENT goestools RUNTIME DESTINATION bin)
target_link_libraries(goesemwin assembler nanomsg packet_reader m stdc++)
