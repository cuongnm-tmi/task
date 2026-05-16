set(FILAMENT_ROOT "" CACHE PATH "Path to a Filament SDK or source build root.")
set(FILAMENT_LIB_DIR "" CACHE PATH "Optional explicit Filament library directory.")
set(FILAMENT_LIBRARIES "" CACHE STRING "Optional semicolon-separated Filament libraries override.")

if(NOT FILAMENT_ROOT AND DEFINED ENV{FILAMENT_ROOT})
    set(FILAMENT_ROOT "$ENV{FILAMENT_ROOT}")
endif()

if(NOT FILAMENT_ROOT AND DEFINED FILAMENT_DIR)
    set(FILAMENT_ROOT "${FILAMENT_DIR}")
endif()

if(NOT FILAMENT_ROOT)
    message(FATAL_ERROR
        "FILAMENT_ROOT is required. Point it at an extracted Filament SDK or a built Filament tree. "
        "Example: cmake -B build -S . -DFILAMENT_ROOT=C:/dev/filament/out/cmake-release")
endif()

file(TO_CMAKE_PATH "${FILAMENT_ROOT}" FILAMENT_ROOT)

set(_filament_include_candidates
    "${FILAMENT_ROOT}/include"
    "${FILAMENT_ROOT}/filament/include"
    "${FILAMENT_ROOT}/libs/gltfio/include"
    "${FILAMENT_ROOT}/libs/utils/include"
    "${FILAMENT_ROOT}/libs/math/include"
    "${FILAMENT_ROOT}/libs/ktxreader/include"
    "${FILAMENT_ROOT}/libs/filabridge/include"
    "${FILAMENT_ROOT}/third_party/robin-map/tnt"
    "${FILAMENT_ROOT}/../../include"
)

find_path(_filament_engine_include filament/Engine.h
    HINTS ${_filament_include_candidates}
    NO_DEFAULT_PATH
)

if(NOT _filament_engine_include)
    message(FATAL_ERROR
        "Could not find filament/Engine.h under FILAMENT_ROOT='${FILAMENT_ROOT}'. "
        "Use an official Filament SDK or pass the source tree root that contains filament/include.")
endif()

set(ICON_MODE_FILAMENT_INCLUDE_DIRS "${_filament_engine_include}")
foreach(_candidate IN LISTS _filament_include_candidates)
    if(EXISTS "${_candidate}")
        list(APPEND ICON_MODE_FILAMENT_INCLUDE_DIRS "${_candidate}")
    endif()
endforeach()
list(REMOVE_DUPLICATES ICON_MODE_FILAMENT_INCLUDE_DIRS)

if(FILAMENT_LIB_DIR)
    file(TO_CMAKE_PATH "${FILAMENT_LIB_DIR}" FILAMENT_LIB_DIR)
else()
    find_library(_filament_engine_library NAMES filament
        HINTS
            "${FILAMENT_ROOT}/lib"
            "${FILAMENT_ROOT}/lib/x86_64"
            "${FILAMENT_ROOT}/lib/x64"
            "${FILAMENT_ROOT}/out/cmake-release/lib"
            "${FILAMENT_ROOT}/out/cmake-release/lib/x86_64"
            "${FILAMENT_ROOT}/out/cmake-release/lib/x64"
            "${FILAMENT_ROOT}/../../lib"
            "${FILAMENT_ROOT}/../../lib/x86_64"
        NO_DEFAULT_PATH
    )
    if(_filament_engine_library)
        get_filename_component(FILAMENT_LIB_DIR "${_filament_engine_library}" DIRECTORY)
    endif()
endif()

if(NOT FILAMENT_LIB_DIR)
    message(FATAL_ERROR
        "Could not locate Filament libraries. Pass -DFILAMENT_LIB_DIR=<path-to-filament-libs>.")
endif()

set(ICON_MODE_FILAMENT_LIBRARY_DIRS "${FILAMENT_LIB_DIR}")

if(FILAMENT_LIBRARIES)
    set(ICON_MODE_FILAMENT_LIBRARIES ${FILAMENT_LIBRARIES})
else()
    set(ICON_MODE_FILAMENT_LIBRARIES
        filament
        gltfio
        filamat
        ktxreader
        imageio
        image
        ibl
        geometry
        backend
        filabridge
        filaflat
        utils
        bluegl
        bluevk
    )
endif()

message(STATUS "Filament includes: ${ICON_MODE_FILAMENT_INCLUDE_DIRS}")
message(STATUS "Filament library dirs: ${ICON_MODE_FILAMENT_LIBRARY_DIRS}")
message(STATUS "Filament libraries: ${ICON_MODE_FILAMENT_LIBRARIES}")

