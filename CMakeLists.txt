cmake_minimum_required(VERSION 3.15)
project(escposPrint C)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic-errors")

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

target_compile_options(escposPrint PRIVATE
  -g -O0
  -fno-omit-frame-pointer
  -fsanitize=address,undefined
  -fno-sanitize-recover=all
  --coverage
)

target_link_options(escposPrint PRIVATE
  -fsanitize=address,undefined
  --coverage
)

# coverage example:
#   build/escposPrint examples/keyboard.png /dev/usb/lp0
#   gcov build/CMakeFiles/escposPrint.dir/load_png.c.gcno
#   -> you see e.g.:
#      File '/home/tux/Dokumente/projects/printer zj-58/EscPos_print/load_png.c'
#      Lines executed:97.22% of 36
#      Creating 'load_png.c.gcov'
#
#      Lines executed:97.22% of 36
#   cat load_png.c.gcov
#   -> you see: complete data about execution for each line!