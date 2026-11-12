cmake_minimum_required(VERSION 3.16)

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum iOS version")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" AND NOT CMAKE_SYSTEM_NAME)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
endif()

project(SuperMarioBros VERSION 1.0.0 LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJC OBJCXX)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(SMB_IOS ON)
else()
    set(SMB_IOS OFF)
endif()

set(SMB_VERSION "${PROJECT_VERSION}" CACHE STRING "Version shown in app bundles")
set(SMB_BUILD_NUMBER "1" CACHE STRING "Build number for app bundles")

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
    src/AudioOutput.cpp
    src/Bus.cpp
    src/Cpu.cpp
    src/Draw.cpp
    src/FramePacer.cpp
    src/Game.cpp
    src/Main.cpp
    src/Ppu.cpp
    src/Rom.cpp
    src/TouchControls.cpp
    src/Zip.cpp
    ${SMB_GENERATED_SOURCE}
)

if(SMB_IOS)
    list(APPEND SMB_SOURCES src/IosPlatform.mm)
    set_source_files_properties(src/IosPlatform.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
endif()

if(ANDROID)
    add_library(main SHARED ${SMB_SOURCES})
    set(SMB_TARGET main)
elseif(APPLE)
    add_executable(SuperMarioBros MACOSX_BUNDLE ${SMB_SOURCES})
    set(SMB_TARGET SuperMarioBros)
    if(SMB_IOS)
        set(SMB_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/platform/ios/Info.plist.in)
    else()
        set(SMB_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/platform/macos/Info.plist.in)
    endif()
    set_target_properties(SuperMarioBros PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${SMB_PLIST}
        MACOSX_BUNDLE_BUNDLE_NAME "Super Mario Bros."
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.brenninhotools.supermariobros"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${SMB_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${SMB_BUILD_NUMBER}"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.brenninhotools.supermariobros"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
    )
    if(SMB_IOS)
        target_link_libraries(SuperMarioBros PRIVATE "-framework UIKit" "-framework UniformTypeIdentifiers")
    endif()
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
