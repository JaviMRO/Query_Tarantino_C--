# C++20 on every compiler (SPEC §11), and the options every target of the project shares through tarantino_options.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT MSVC)
    # SPEC §11: built in release mode with -O2 -DNDEBUG (CMake's Release would use -O3)
    set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
endif()

add_library(tarantino_options INTERFACE)

if(MSVC)
    # Source files are UTF-8, and Windows headers come without their min/max macros
    target_compile_options(tarantino_options INTERFACE /utf-8 /permissive- /Zc:__cplusplus /EHsc /bigobj /W4
                           /wd4100 /wd4996)
else()
    target_compile_options(tarantino_options INTERFACE -Wall -Wextra)
endif()

if(WIN32)
    # Windows 10 or later: its icu.dll (version 1903) and the wide-character file APIs used for long paths
    target_compile_definitions(tarantino_options INTERFACE NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS
                               _WIN32_WINNT=0x0A00)
    if(MINGW)
        # Executables that run without the DLLs of the MinGW toolchain
        target_link_options(tarantino_options INTERFACE -static)
    endif()
endif()
