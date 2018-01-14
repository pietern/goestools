add_library(publisher publisher.cc)
target_link_libraries(publisher nanomsg)

add_library(airspy_source airspy_source.cc)
target_link_libraries(airspy_source airspy publisher stdc++)

add_library(rtlsdr_source rtlsdr_source.cc)
target_link_libraries(rtlsdr_source rtlsdr publisher stdc++)

add_library(agc agc.cc)
target_link_libraries(agc stdc++)

add_library(rrc rrc.cc)
target_link_libraries(rrc stdc++)
