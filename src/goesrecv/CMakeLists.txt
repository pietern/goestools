add_library(publisher publisher.cc)
target_link_libraries(publisher nanomsg)

add_library(airspy_source airspy_source.cc)
target_link_libraries(airspy_source airspy publisher stdc++)

add_library(rtlsdr_source rtlsdr_source.cc)
target_link_libraries(rtlsdr_source rtlsdr publisher stdc++)

add_library(agc agc.cc)
target_link_libraries(agc m stdc++)

add_library(rrc rrc.cc)
target_link_libraries(rrc stdc++)

add_library(costas costas.cc)
target_link_libraries(costas stdc++)

add_library(clock_recovery clock_recovery.cc)
target_link_libraries(clock_recovery stdc++)

add_library(quantize quantize.cc)
target_link_libraries(quantize stdc++)

add_executable(goesrecv goesrecv.cc options.cc decoder.cc demodulator.cc)
target_include_directories(goesrecv PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(goesrecv packetizer pthread publisher)
target_link_libraries(goesrecv airspy_source)
target_link_libraries(goesrecv rtlsdr_source)
target_link_libraries(goesrecv agc)
target_link_libraries(goesrecv rrc)
target_link_libraries(goesrecv costas)
target_link_libraries(goesrecv clock_recovery)
target_link_libraries(goesrecv quantize)
