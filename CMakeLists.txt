cmake_minimum_required(VERSION 3.16)


set( CMAKE_C_COMPILER "gcc" )

project(PwCDemoSplash2025 LANGUAGES C VERSION 3.16)

set(VULKAN_FOLDER "1.4.328.0")
set(ENV{VULKAN_SDK} "C:/VulkanSDK/${VULKAN_FOLDER}")

find_package(Vulkan REQUIRED)

set(CMAKE_C_STANDARD 23)
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

set(SRC
		src/main.c
		src/vulkan.c
		src/vulkan.h
)

add_executable(${PROJECT_NAME} ${SRC} ${LIBRARIES})

target_include_directories(${PROJECT_NAME} PUBLIC "${VULKAN_SDK}/Include")
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan SDL3::SDL3)