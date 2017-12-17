add_library(packetizer
  correlator.cc
  derandomizer.cc
  reed_solomon.cc
  packetizer.cc
  reader.cc
  )
target_link_libraries(packetizer correct_static)

add_executable(goesdec
  crc.cc
  file_handler.cc
  main.cc
  session_pdu.cc
  transport_pdu.cc
  virtual_channel.cc
  )
add_sanitizers(goesdec)
target_link_libraries(goesdec lrit packetizer aec sz)

add_executable(compute_sync_words compute_sync_words.cc)
add_sanitizers(compute_sync_words)
target_link_libraries(compute_sync_words correct_static)
