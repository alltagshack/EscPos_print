cmake_minimum_required(VERSION 3.15)
project(miniMidiOut C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

find_library(PORTAUDIO_LIBRARY portaudio)
if(NOT PORTAUDIO_LIBRARY)
    message(FATAL_ERROR
        "PortAudio not found.\n"
        "Install the Debian package: portaudio19-dev.")
endif()

find_package(Threads QUIET)
if(NOT Threads_FOUND)
    message(FATAL_ERROR
        "POSIX threads library not found.\n"
        "Install the Debian package: libpthread-stubs0-dev.")
endif()

find_library(MATH_LIBRARY m)
if(NOT MATH_LIBRARY)
    message(FATAL_ERROR
        "Math library (libm) not found.\n"
        "Install the Debian package: libc6-dev (contains libm).")
endif()

add_executable(miniMidiOut
    main.c
    modus.c
    pseudo_random.c
    time_tools.c
    voice.c
    noise.c
    device.c
    device/keyboard.c
    device/midi.c
    player.c
    globals.c)

target_include_directories(miniMidiOut PRIVATE
    include)

target_link_libraries(miniMidiOut
    ${PORTAUDIO_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${MATH_LIBRARY})
