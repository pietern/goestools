add_executable(goesdec
  correlator.cc
  crc.cc
  derandomizer.cc
  file_handler.cc
  main.cc
  reader.cc
  reed_solomon.cc
  session_pdu.cc
  transport_pdu.cc
  virtual_channel.cc
  )
add_sanitizers(goesdec)
target_link_libraries(goesdec lrit correct_static aec sz)

add_executable(compute_sync_words compute_sync_words.cc)
add_sanitizers(compute_sync_words)
target_link_libraries(compute_sync_words correct_static)
