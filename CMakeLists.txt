cmake_minimum_required(VERSION 3.16)

project(smolnes)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")

option(SMOLNES_VENDORED "Use vendored dependencies" ${WIN32})

if(SMOLNES_VENDORED)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")
    add_subdirectory(SDL EXCLUDE_FROM_ALL)
else()
    find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
    find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)
endif()

add_executable(smolnes WIN32 src/org/smolnes.c)
add_executable(deobfuscated WIN32 src/org/deobfuscated.c)
if(WIN32)
    add_executable(deobfuscated_win32 WIN32 src/deobfuscated_win32.c)
endif()

if(TARGET SDL2::SDL2main)
    target_link_libraries(smolnes PRIVATE SDL2::SDL2main)
    target_link_libraries(deobfuscated PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(smolnes PRIVATE SDL2::SDL2)
target_link_libraries(deobfuscated PRIVATE SDL2::SDL2)
if(TARGET deobfuscated_win32)
    target_link_libraries(deobfuscated_win32 PRIVATE user32 gdi32 comdlg32)
    if(MINGW)
        target_compile_options(deobfuscated_win32 PRIVATE -Os -ffunction-sections)
        target_link_options(deobfuscated_win32 PRIVATE -Wl,--gc-sections -s)
    endif()
endif()
