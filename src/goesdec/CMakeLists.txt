add_executable(packetinfo packetinfo.cc)
target_link_libraries(packetinfo stdc++)

add_executable(goesdec
  crc.cc
  file_handler.cc
  main.cc
  session_pdu.cc
  transport_pdu.cc
  virtual_channel.cc
  )
add_sanitizers(goesdec)
target_link_libraries(goesdec lrit aec sz stdc++)
