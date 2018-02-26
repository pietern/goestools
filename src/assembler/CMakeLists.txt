add_executable(packetinfo packetinfo.cc)
target_link_libraries(packetinfo m stdc++)

add_library(assembler
  assembler.cc
  crc.cc
  session_pdu.cc
  transport_pdu.cc
  virtual_channel.cc
  )
target_link_libraries(assembler lrit aec sz stdc++)
