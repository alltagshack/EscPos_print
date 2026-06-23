cmake_minimum_required(VERSION 3.15)
project(escposPrint C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

find_package(PNG REQUIRED)
if(NOT PNG_LIBRARY)
    message(FATAL_ERROR
        "libpng not found.\n"
        "Install the Debian package: libpng-dev.")
endif()

add_executable(escposPrint
    load_bmp.c
    load_png.c
    printing.c
    markdown.c
    main.c)

target_include_directories(escposPrint PRIVATE
    ${PNG_INCLUDE_DIR}
    include)

target_link_libraries(escposPrint
    ${PNG_LIBRARY})
