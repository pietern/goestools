add_library(lrit lrit.cc file.cc)
add_executable(lritdump lritdump.cc)
add_sanitizers(lritdump)
target_link_libraries(lritdump lrit stdc++)

add_library(dcs dcs.cc)
add_executable(dcsdump dcsdump.cc)
add_sanitizers(dcsdump)
target_link_libraries(dcsdump lrit dcs stdc++)

add_library(util util.cc)
