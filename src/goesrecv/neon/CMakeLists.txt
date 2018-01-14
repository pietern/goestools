set(CMAKE_C_FLAGS "-std=c11 -mfpu=neon -O3")
add_executable(neon_mathfun_test neon_mathfun_test.c)
target_link_libraries(neon_mathfun_test m)
