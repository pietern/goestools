add_library(dcs dcs.cc)

add_executable(dcsdump dcsdump.cc)
add_sanitizers(dcsdump)
target_link_libraries(dcsdump lrit dcs m stdc++)
