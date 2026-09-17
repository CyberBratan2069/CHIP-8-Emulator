cmake_minimum_required(VERSION 3.15)
project(CHIP_8_Emulator C)

set(CMAKE_C_STANDARD 11)

find_package(raylib REQUIRED)

add_executable(CHIP_8_Emulator src/main.c src/cpu.c)

target_link_libraries(CHIP_8_Emulator PRIVATE raylib)

if (APPLE)
    target_link_libraries(CHIP_8_Emulator PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
endif()
