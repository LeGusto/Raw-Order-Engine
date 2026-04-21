cmake_minimum_required(VERSION 3.20)
project(OrderBook CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_compile_options(-Wall -Wextra -Werror)

add_compile_options(
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)
add_link_options(
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

add_compile_options(
    $<$<CONFIG:TSan>:-g>
    $<$<CONFIG:TSan>:-O1>
    $<$<CONFIG:TSan>:-fsanitize=thread>
)
add_link_options(
    $<$<CONFIG:TSan>:-fsanitize=thread>
)

add_compile_options($<$<CONFIG:Release>:-O2>)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/includes ${CMAKE_SOURCE_DIR}/src)

find_package(Threads REQUIRED)

add_executable(server
    src/server_main.cpp
    src/server_base.cpp
    src/server_tcp.cpp
    src/server_udp.cpp
    src/serializer.cpp
    src/order_book.cpp
)
target_compile_definitions(server PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

add_executable(user
    src/user_spawner.cpp
    src/user.cpp
    src/serializer.cpp
)
target_link_libraries(user PRIVATE Threads::Threads)

enable_testing()

add_executable(test_order_book
    tests/test_order_book.cpp
    src/order_book.cpp
)
add_test(NAME test_order_book COMMAND test_order_book)

add_executable(test_serializer
    tests/test_serializer.cpp
    src/serializer.cpp
)
add_test(NAME test_serializer COMMAND test_serializer)

add_custom_target(run_server
    COMMAND $<TARGET_FILE:server>
    DEPENDS server
    USES_TERMINAL
)

add_custom_target(run_user
    COMMAND $<TARGET_FILE:user>
    DEPENDS user
    USES_TERMINAL
)
