cmake_minimum_required(VERSION 3.15)
project(snub VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /O2)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

set(SOURCES
    src/search.c
    src/cli.c
    src/utils.c
    main.c
)

add_executable(snub ${SOURCES})

target_include_directories(snub PRIVATE src)

if(WIN32)
    target_link_libraries(snub)
endif()

enable_testing()

install(TARGETS snub DESTINATION bin)
