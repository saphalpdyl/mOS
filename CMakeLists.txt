cmake_minimum_required(VERSION 4.4)
project(mos C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(mlib OBJECT
        include/mos/lib/core/buffer.h
        include/mos/lib/core/pool.h
        include/mos/lib/core/fifo.h
        src/lib/core/buffer.c
        src/lib/core/pool.c
        include/mos/lib/graph/frame.h
        include/mos/lib/graph/node.h
        include/mos/lib/graph/worker.h
        src/lib/graph/frame.c
        src/lib/graph/node.c
        src/lib/graph/worker.c
        src/lib/graph/drop.c
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
        tests/lib/test_pool.c
        tests/lib/test_buffer.c
        tests/test_common.c
        tests/nodes/send.c
        tests/lib/test_graph.c
)

target_include_directories(fwd_tests PUBLIC tests)
target_link_libraries(fwd_tests PRIVATE mlib)
target_compile_options(fwd_tests PRIVATE -g)
target_link_options(fwd_tests PRIVATE)

enable_testing()
add_test(NAME fwd_tests COMMAND fwd_tests)
