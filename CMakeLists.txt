cmake_minimum_required(VERSION 3.25)

project(icon_mode_store VERSION 0.2.0 LANGUAGES CXX)

option(ICON_MODE_BUILD_VIEWER "Build the C++ Filament walkthrough viewer." ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT ICON_MODE_BUILD_VIEWER)
    message(STATUS "ICON_MODE_BUILD_VIEWER=OFF; only project files are configured.")
    return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FilamentSdk)

find_package(SDL2 CONFIG REQUIRED)

add_executable(icon_mode_store
    src/main.cpp
    src/CameraController.cpp
    src/LightManager.cpp
    src/StoreApp.cpp
    src/StoreScene.cpp
)

target_include_directories(icon_mode_store
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        ${ICON_MODE_FILAMENT_INCLUDE_DIRS}
)

target_link_directories(icon_mode_store PRIVATE ${ICON_MODE_FILAMENT_LIBRARY_DIRS})

target_link_libraries(icon_mode_store
    PRIVATE
        SDL2::SDL2
        ${ICON_MODE_FILAMENT_LIBRARIES}
)

if(TARGET SDL2::SDL2main)
    target_link_libraries(icon_mode_store PRIVATE SDL2::SDL2main)
endif()

if(MSVC)
    target_compile_options(icon_mode_store PRIVATE /W3 /permissive-)
    target_compile_definitions(icon_mode_store PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    target_compile_options(icon_mode_store PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_custom_command(TARGET icon_mode_store POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:icon_mode_store>/assets"
    VERBATIM
)

