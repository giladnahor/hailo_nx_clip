## Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

cmake_minimum_required(VERSION 3.15)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
project(clip_person_tracker_plugin)

set(metadataSdkVersion "5.0.0.35270")
set(metadataSdkDir "" CACHE PATH
    "Path to unpacked VMS Metadata SDK zip. Tested with version ${metadataSdkVersion}.")

if(metadataSdkDir STREQUAL "")
    message(FATAL_ERROR "metadataSdkDir cache variable is undefined.")
endif()

if(NOT DEFINED ARTIFACTORY_URL)
    set(ARTIFACTORY_URL "https://artifactory.nxvms.dev/artifactory/artifacts/nx_open_integrations")
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(UNIX)
    # In Linux, for the plugin .so library, set `rpath` to "$ORIGIN" and do not set `runpath`, thus
    # enabling the lookup of the dependencies in the plugin dir first.
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--disable-new-dtags")
endif()
set(CMAKE_SKIP_BUILD_RPATH ON)
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
set(CMAKE_INSTALL_RPATH "$ORIGIN")

if(WIN32)
    set(API_EXPORT_MACRO "__declspec(dllexport)")
elseif(UNIX)
    set(API_EXPORT_MACRO "__attribute__((visibility(\"default\")))")
endif()

#--------------------------------------------------------------------------------------------------
# Define nx_kit lib, static.

set(nxKitLibraryType "STATIC" CACHE STRING "" FORCE)
set(nxKitWithTests "YES" CACHE STRING "" FORCE)

add_subdirectory(${metadataSdkDir}/nx_kit ${CMAKE_CURRENT_BINARY_DIR}/nx_kit)

#--------------------------------------------------------------------------------------------------
# Define nx_sdk lib, static, depends on nx_kit.

set(SDK_SRC_DIR ${metadataSdkDir}/src)
file(GLOB_RECURSE SDK_SRC ${SDK_SRC_DIR}/*)

add_library(nx_sdk STATIC ${SDK_SRC})
target_include_directories(nx_sdk PUBLIC ${SDK_SRC_DIR})
target_link_libraries(nx_sdk PRIVATE nx_kit)

target_compile_definitions(nx_sdk PRIVATE NX_PLUGIN_API=${API_EXPORT_MACRO})
#--------------------------------------------------------------------------------------------------
# Add TAPPAS dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(TAPPAS REQUIRED IMPORTED_TARGET hailo_tappas)
# target_link_libraries(clip_person_tracker_plugin PUBLIC PkgConfig::TAPPAS)
#include_directories(/home/giladn/HAILO_SUITE/2023-10-Suite/hailo_ai_sw_suite/hailo_venv/lib/python3.10/site-packages/tensorflow/include/external/local_config_python/python_include)
# include_directories(/home/giladn/HAILO_SUITE/2023-10-Suite/hailo_ai_sw_suite/artifacts/tappas/sources/xtensor-blas/include)
# opencv libraries
find_package(OpenCV REQUIRED)
find_package(BLAS REQUIRED)

#--------------------------------------------------------------------------------------------------
# Define clip_person_tracker_plugin lib, dynamic, depends on nx_kit and nx_sdk.

set(clip_person_tracker_plugin_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
file(GLOB_RECURSE clip_person_tracker_plugin_SRC
    ${clip_person_tracker_plugin_SRC_DIR}/*)

add_library(clip_person_tracker_plugin SHARED
    ${clip_person_tracker_plugin_SRC})
target_include_directories(clip_person_tracker_plugin PRIVATE 
    ${OPENCV_ANALYTICS_PLUGIN_SRC_DIR})
set(CMAKE_EXE_LINKER_FLAGS " -static")
target_link_libraries(clip_person_tracker_plugin
    nx_kit
    nx_sdk
    PkgConfig::TAPPAS
    ${BLAS_LIBRARIES}
    # opencv::core opencv::flann opencv::imgproc opencv::imgcodecs opencv::dnn opencv::opencv_dnn opencv::ml 
    # opencv::plot opencv::opencv_features2d opencv::opencv_calib3d opencv::datasets opencv::video opencv::tracking
)

target_compile_definitions(clip_person_tracker_plugin
    PRIVATE NX_PLUGIN_API=${API_EXPORT_MACRO}
)
