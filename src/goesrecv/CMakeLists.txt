add_library(publisher
  packet_publisher.cc
  publisher.cc
  sample_publisher.cc
  soft_bit_publisher.cc
  stats_publisher.cc
  )
target_link_libraries(publisher nanomsg)

find_library(AIRSPY_LIBRARY
  NAMES libairspy.so
  PATH_SUFFIXES arm-linux-gnueabihf)
if(NOT AIRSPY_LIBRARY)
  message(FATAL_ERROR "Unable to find libairspy")
endif()

find_library(RTLSDR_LIBRARY
  NAMES librtlsdr.so
  PATH_SUFFIXES arm-linux-gnueabihf)
if(NOT RTLSDR_LIBRARY)
  message(FATAL_ERROR "Unable to find librtlsdr")
endif()

add_library(airspy_source airspy_source.cc)
target_link_libraries(airspy_source ${AIRSPY_LIBRARY} publisher stdc++)

add_library(nanomsg_source nanomsg_source.cc)
target_link_libraries(nanomsg_source nanomsg publisher stdc++)

add_library(rtlsdr_source rtlsdr_source.cc)
target_link_libraries(rtlsdr_source ${RTLSDR_LIBRARY} publisher stdc++)

add_library(agc agc.cc)
target_link_libraries(agc publisher m stdc++)

add_library(rrc rrc.cc)
target_link_libraries(rrc publisher stdc++)

add_library(costas costas.cc)
target_link_libraries(costas publisher stdc++)

add_library(clock_recovery clock_recovery.cc)
target_link_libraries(clock_recovery publisher stdc++)

add_library(quantize quantize.cc)
target_link_libraries(quantize publisher stdc++)

add_executable(goesrecv goesrecv.cc config.cc options.cc decoder.cc demodulator.cc)
target_include_directories(goesrecv PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(goesrecv packetizer pthread)
target_link_libraries(goesrecv airspy_source)
target_link_libraries(goesrecv nanomsg_source)
target_link_libraries(goesrecv rtlsdr_source)
target_link_libraries(goesrecv agc)
target_link_libraries(goesrecv rrc)
target_link_libraries(goesrecv costas)
target_link_libraries(goesrecv clock_recovery)
target_link_libraries(goesrecv quantize)

add_executable(benchmark benchmark.cc)
target_link_libraries(benchmark pthread)
target_link_libraries(benchmark agc)
target_link_libraries(benchmark rrc)
target_link_libraries(benchmark costas)
target_link_libraries(benchmark clock_recovery)
