add_library(lrit lrit.cc file.cc)

add_executable(lritdump lritdump.cc)
add_sanitizers(lritdump)
target_link_libraries(lritdump lrit m stdc++)

add_executable(areadump areadump.cc)
add_sanitizers(areadump)
target_link_libraries(areadump lrit dir m stdc++)
