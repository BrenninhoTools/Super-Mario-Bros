cmake_minimum_required(VERSION 3.16)

project(SuperMarioBros VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(SMB_SYSTEM_SDL "Use the SDL2 installed on the system instead of external/SDL" OFF)
option(SMB_COMPILER_CACHE "Use sccache or ccache when available" ON)

if(SMB_COMPILER_CACHE AND NOT CMAKE_CXX_COMPILER_LAUNCHER)
    if(MSVC)
        find_program(SMB_CACHE_PROGRAM NAMES sccache ccache)
    else()
        find_program(SMB_CACHE_PROGRAM NAMES ccache sccache)
    endif()
    if(SMB_CACHE_PROGRAM)
        set(CMAKE_C_COMPILER_LAUNCHER ${SMB_CACHE_PROGRAM})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${SMB_CACHE_PROGRAM})
        message(STATUS "Compiler cache: ${SMB_CACHE_PROGRAM}")
    endif()
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(SMB_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(SMB_GENERATED_SOURCE ${SMB_GENERATED_DIR}/SmbGame.cpp)

add_custom_command(
    OUTPUT ${SMB_GENERATED_SOURCE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/smb2cpp.py
            ${CMAKE_CURRENT_SOURCE_DIR}/asm/smbdis.asm
            ${SMB_GENERATED_SOURCE}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/smb2cpp.py
            ${CMAKE_CURRENT_SOURCE_DIR}/asm/smbdis.asm
    COMMENT "Translating smbdis.asm to C++"
    VERBATIM
)

if(SMB_SYSTEM_SDL)
    find_package(SDL2 REQUIRED)
else()
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/external/SDL/CMakeLists.txt)
        message(FATAL_ERROR "external/SDL is missing. Run: git submodule update --init --recursive")
    endif()
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_CCACHE OFF CACHE BOOL "" FORCE)
    set(SDL2_DISABLE_UNINSTALL ON CACHE BOOL "" FORCE)
    if(ANDROID)
        set(SDL_SHARED ON CACHE BOOL "" FORCE)
        set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    else()
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
        set(SDL_STATIC_PIC ON CACHE BOOL "" FORCE)
    endif()
    if(ANDROID)
        add_subdirectory(external/SDL)
    else()
        add_subdirectory(external/SDL EXCLUDE_FROM_ALL)
    endif()
endif()

set(SMB_SOURCES
    src/Apu.cpp
    src/Bus.cpp
    src/Cpu.cpp
    src/Draw.cpp
    src/Game.cpp
    src/Main.cpp
    src/Ppu.cpp
    src/Rom.cpp
    src/TouchControls.cpp
    ${SMB_GENERATED_SOURCE}
)

if(ANDROID)
    add_library(main SHARED ${SMB_SOURCES})
    set(SMB_TARGET main)
else()
    add_executable(SuperMarioBros WIN32 ${SMB_SOURCES})
    set(SMB_TARGET SuperMarioBros)
endif()

target_include_directories(${SMB_TARGET} PRIVATE src)

if(MSVC)
    target_compile_options(${SMB_TARGET} PRIVATE /W3 /bigobj)
    target_compile_definitions(${SMB_TARGET} PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(${SMB_TARGET} PRIVATE -Wall -Wno-unused-label)
endif()

if(SMB_SYSTEM_SDL)
    if(TARGET SDL2::SDL2main)
        target_link_libraries(${SMB_TARGET} PRIVATE SDL2::SDL2main)
    endif()
    target_link_libraries(${SMB_TARGET} PRIVATE SDL2::SDL2)
elseif(ANDROID)
    target_link_libraries(${SMB_TARGET} PRIVATE SDL2::SDL2 android log)
else()
    target_link_libraries(${SMB_TARGET} PRIVATE SDL2::SDL2main SDL2::SDL2-static)
endif()
