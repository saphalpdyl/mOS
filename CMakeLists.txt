cmake_minimum_required(VERSION 4.4)
project(mos C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(fwdcore STATIC
        include/mos/buffer.h
        src/core/buffer.c
)
target_include_directories(fwdcore PUBLIC include)
target_compile_options(fwdcore PUBLIC -Wall -Wextra -Wshadow -fsanitize=address,undefined -g)
target_link_options(fwdcore PUBLIC -fsanitize=address,undefined)

add_executable(mos main.c)
target_link_libraries(mos PRIVATE fwdcore)

#Host-side tests
add_executable(fwd_tests
        tests/test_main.c
        include/mos/buffer.h
)

target_link_libraries(fwd_tests PRIVATE fwdcore)
target_compile_options(fwd_tests PRIVATE -g)
target_link_options(fwd_tests PRIVATE)

enable_testing()
add_test(NAME fwd_tests COMMAND fwd_tests)
