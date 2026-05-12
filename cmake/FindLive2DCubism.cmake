# Locate a local Live2D Cubism SDK for Native installation.
#
# This finder is intentionally narrow: MemoChat does not vendor the SDK and the
# default desktop pet renderer must keep working without it. Callers should only
# use this module from an explicit opt-in path such as MEMOCHAT_ENABLE_LIVE2D_NATIVE.
#
# Input hints:
#   Live2DCubism_ROOT
#   LIVE2DCUBISM_ROOT
#   ENV{MEMOCHAT_LIVE2D_SDK_ROOT}
#   ENV{LIVE2DCUBISM_ROOT}
#
# Result variables:
#   Live2DCubism_FOUND
#   Live2DCubism_ROOT_DIR
#   Live2DCubism_CORE_INCLUDE_DIR
#   Live2DCubism_FRAMEWORK_INCLUDE_DIR
#   Live2DCubism_CORE_LIBRARY
#   Live2DCubism_INCLUDE_DIRS
#   Live2DCubism_LIBRARIES
#
# Imported targets:
#   Live2DCubism::Core
#   Live2DCubism::Framework
#   Live2DCubism::Native

include(FindPackageHandleStandardArgs)

set(_Live2DCubism_ROOT_HINTS
    "${Live2DCubism_ROOT}"
    "${LIVE2DCUBISM_ROOT}"
    "$ENV{MEMOCHAT_LIVE2D_SDK_ROOT}"
    "$ENV{LIVE2DCUBISM_ROOT}"
)

foreach(_Live2DCubism_ROOT_HINT IN LISTS _Live2DCubism_ROOT_HINTS)
    if(_Live2DCubism_ROOT_HINT)
        file(TO_CMAKE_PATH "${_Live2DCubism_ROOT_HINT}" _Live2DCubism_ROOT_HINT_NORMALIZED)
        list(APPEND _Live2DCubism_NORMALIZED_ROOT_HINTS "${_Live2DCubism_ROOT_HINT_NORMALIZED}")
    endif()
endforeach()

find_path(Live2DCubism_CORE_INCLUDE_DIR
    NAMES Live2DCubismCore.hpp
    HINTS ${_Live2DCubism_NORMALIZED_ROOT_HINTS}
    PATH_SUFFIXES
        Core/include
        include
    NO_DEFAULT_PATH
)

find_path(Live2DCubism_FRAMEWORK_INCLUDE_DIR
    NAMES CubismFramework.hpp
    HINTS ${_Live2DCubism_NORMALIZED_ROOT_HINTS}
    PATH_SUFFIXES
        Framework/src
        Framework/include
        Framework
    NO_DEFAULT_PATH
)

find_library(Live2DCubism_CORE_LIBRARY
    NAMES Live2DCubismCore libLive2DCubismCore
    HINTS ${_Live2DCubism_NORMALIZED_ROOT_HINTS}
    PATH_SUFFIXES
        Core/lib/linux/x86_64
        Core/lib/linux
        Core/lib/macos/arm64
        Core/lib/macos/x86_64
        Core/lib/windows/x86_64
        Core/lib/windows/x86
        Core/lib
        lib
    NO_DEFAULT_PATH
)

if(Live2DCubism_CORE_INCLUDE_DIR)
    get_filename_component(_Live2DCubism_CORE_DIR "${Live2DCubism_CORE_INCLUDE_DIR}/.." ABSOLUTE)
    get_filename_component(Live2DCubism_ROOT_DIR "${_Live2DCubism_CORE_DIR}/.." ABSOLUTE)
elseif(_Live2DCubism_NORMALIZED_ROOT_HINTS)
    list(GET _Live2DCubism_NORMALIZED_ROOT_HINTS 0 Live2DCubism_ROOT_DIR)
endif()

find_package_handle_standard_args(Live2DCubism
    REQUIRED_VARS
        Live2DCubism_CORE_INCLUDE_DIR
        Live2DCubism_FRAMEWORK_INCLUDE_DIR
        Live2DCubism_CORE_LIBRARY
)

if(Live2DCubism_FOUND)
    set(Live2DCubism_INCLUDE_DIRS
        "${Live2DCubism_CORE_INCLUDE_DIR}"
        "${Live2DCubism_FRAMEWORK_INCLUDE_DIR}"
    )
    set(Live2DCubism_LIBRARIES "${Live2DCubism_CORE_LIBRARY}")

    if(NOT TARGET Live2DCubism::Core)
        add_library(Live2DCubism::Core UNKNOWN IMPORTED)
        set_target_properties(Live2DCubism::Core PROPERTIES
            IMPORTED_LOCATION "${Live2DCubism_CORE_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${Live2DCubism_CORE_INCLUDE_DIR}"
        )
    endif()

    if(NOT TARGET Live2DCubism::Framework)
        add_library(Live2DCubism::Framework INTERFACE IMPORTED)
        set_target_properties(Live2DCubism::Framework PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${Live2DCubism_FRAMEWORK_INCLUDE_DIR}"
        )
    endif()

    if(NOT TARGET Live2DCubism::Native)
        add_library(Live2DCubism::Native INTERFACE IMPORTED)
        target_link_libraries(Live2DCubism::Native INTERFACE
            Live2DCubism::Framework
            Live2DCubism::Core
        )
    endif()
endif()

mark_as_advanced(
    Live2DCubism_CORE_INCLUDE_DIR
    Live2DCubism_FRAMEWORK_INCLUDE_DIR
    Live2DCubism_CORE_LIBRARY
)
