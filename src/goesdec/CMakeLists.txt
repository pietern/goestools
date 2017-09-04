file(GLOB GOESDEC_SRCS *.cc)
add_executable(goesdec ${GOESDEC_SRCS})
add_sanitizers(goesdec)
target_link_libraries(goesdec lrit correct_static aec sz)
