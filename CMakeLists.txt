cmake_minimum_required(VERSION 3.16)

set( CMAKE_C_COMPILER "gcc" )

project(PwCDemoSplash2025 LANGUAGES C VERSION 3.16)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if (UNIX)
        set(SDL_VIDEO_DRIVER x11)
        set(SDL_WAYLAND OFF)
        set(SDL_WAYLAND_SHARED OFF)
        set(SDL_UNIX_CONSOLE_BUILD ON)
endif (UNIX)
add_subdirectory(libs/SDL EXCLUDE_FROM_ALL)

include_directories(
        src
        libs/SDL/include
)

add_executable(${PROJECT_NAME} src/main.c ${LIBRARIES})

target_link_libraries(${PROJECT_NAME} SDL3::SDL3)