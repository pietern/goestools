add_library(packetizer
  correlator.cc
  derandomizer.cc
  reed_solomon.cc
  packetizer.cc
  reader.cc
  )
target_link_libraries(packetizer correct_static)

add_executable(packetinfo packetinfo.cc)
target_link_libraries(packetinfo stdc++)

add_executable(packetdump packetdump.cc)
add_sanitizers(packetdump)
target_link_libraries(packetdump packetizer stdc++)

set(GOESDEC_SRCS
  crc.cc
  file_handler.cc
  main.cc
  session_pdu.cc
  transport_pdu.cc
  virtual_channel.cc
  )

find_package(OpenSSL)
if(OPENSSL_FOUND)
  list(APPEND GOESDEC_SRCS sha_handler.cc)
endif()

add_executable(goesdec ${GOESDEC_SRCS})
add_sanitizers(goesdec)
target_link_libraries(goesdec lrit aec sz stdc++)

if(OPENSSL_FOUND)
  target_link_libraries(goesdec ${OPENSSL_CRYPTO_LIBRARY})
  target_compile_definitions(goesdec PRIVATE HAVE_OPENSSL)
endif()

add_executable(compute_sync_words compute_sync_words.cc)
add_sanitizers(compute_sync_words)
target_link_libraries(compute_sync_words correct_static stdc++)
