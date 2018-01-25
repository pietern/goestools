add_executable(goesdec
  file_handler.cc
  main.cc
  )
add_sanitizers(goesdec)
target_link_libraries(goesdec lrit assembler stdc++)
