cmake_minimum_required(VERSION 3.15)
project(CHIP_8_Emulator C)

set(CMAKE_C_STANDARD 11)

# Sucht nach einer bereits installierten Raylib-Version auf deinem PC
find_package(raylib REQUIRED)

# Dein Emulator
add_executable(CHIP_8_Emulator main.c)

# Raylib verlinken
target_link_libraries(CHIP_8_Emulator PRIVATE raylib)

# Plattform-spezifische Extras (wichtig für macOS, unter Linux/Windows meist nicht nötig)
if (APPLE)
    target_link_libraries(CHIP_8_Emulator PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
endif()
