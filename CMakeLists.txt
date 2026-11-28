cmake_minimum_required(VERSION 4.4)
project(mos C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(mlib OBJECT
        include/mos/core/buffer.h
        src/core/buffer.c
        include/mos/core/pool.h
        src/core/pool.c
        src/nodes/frame.c
        include/mos/nodes/frame.h
        include/mos/nodes/node.h
        src/nodes/node.c
        include/mos/nodes/worker.h
        include/mos/core/fifo.h
        src/nodes/worker.c
        src/nodes/drop.c
)
target_include_directories(mlib PUBLIC include)
target_compile_options(mlib PUBLIC -Wall -Wextra -Wshadow -Werror -fsanitize=address,undefined -g)
target_link_options(mlib PUBLIC -fsanitize=address,undefined)

add_executable(mos main.c)
target_link_libraries(mos PRIVATE mlib)

#Host-side tests
add_executable(fwd_tests
        tests/test_main.c
        tests/lsan_debugger.c
        tests/test_pool.c
        tests/test_buffer.c
        tests/test_common.c
        tests/nodes/send.c
        tests/test_graph.c
)

target_include_directories(fwd_tests PUBLIC tests)
target_link_libraries(fwd_tests PRIVATE mlib)
target_compile_options(fwd_tests PRIVATE -g)
target_link_options(fwd_tests PRIVATE)

enable_testing()
add_test(NAME fwd_tests COMMAND fwd_tests)
