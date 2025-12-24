cmake_minimum_required(VERSION 3.14)

# Project Name
project(chess VERSION 1.0 LANGUAGES C)

# --- 1. C Standard (C17) ---
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- 2. Install/Fetch Raylib Automatically ---
include(FetchContent)

message(STATUS "Fetching Raylib...")
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(raylib)

# --- 3. Source Files ---
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(SOURCES
    ${SRC_DIR}/main.c
    ${SRC_DIR}/draw.c
    ${SRC_DIR}/load.c
    ${SRC_DIR}/save.c
    ${SRC_DIR}/move.c
    ${SRC_DIR}/colors.c
    ${SRC_DIR}/hash.c
    ${SRC_DIR}/stack.c
    ${SRC_DIR}/utils.c
)

# --- 4. Define Executable ---
add_executable(${PROJECT_NAME} ${SOURCES})

# --- 5. Include Directories ---
# Add 'src' and 'includes' to include path
# 'src' for internal headers, 'includes' for raygui.h and style_amber.h
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${SRC_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/includes"
)

# --- 6. Compiler Flags & Build Modes ---
# Common flags (-Wall -Wextra)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)

# Debug Configuration (-g -O0 -DDEBUG -fsanitize=address,undefined)
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:-g;-O0;-DDEBUG;-fsanitize=address,undefined>
)
# Linker flags for Sanitizers in Debug mode
target_link_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

# Release Configuration (-O3 -s)
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:-O3>
)
# Strip symbols in Release mode
target_link_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:-s>
)

# --- 7. Output Names ---
# Match Makefile behavior: 'chess' for Release, 'debugChess' for Debug
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME_RELEASE "chess"
    OUTPUT_NAME_DEBUG "debugChess"
)

# --- 8. Linking ---
# Link Raylib (It handles GL, m, pthread, X11, etc. automatically)
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# --- 9. Assets Copying ---
# Copy the assets folder to the build directory so the game can find images
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
