add_library(lrit lrit.cc file.cc)

add_executable(lritdump lritdump.cc)
add_sanitizers(lritdump)
target_link_libraries(lritdump lrit m stdc++)
