add_executable(compute_sync_words compute_sync_words.cc)
add_sanitizers(compute_sync_words)
target_link_libraries(compute_sync_words correct_static m stdc++)

add_library(packetizer
  correlator.cc
  derandomizer.cc
  packetizer.cc
  reader.cc
  reed_solomon.cc
  )
target_link_libraries(packetizer correct_static)

add_executable(packetdump packetdump.cc)
add_sanitizers(packetdump)
target_link_libraries(packetdump packetizer m stdc++)
