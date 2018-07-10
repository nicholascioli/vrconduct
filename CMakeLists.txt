cmake_minimum_required(VERSION 3.9)

project(graphics)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
set(BSF_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/bsf" CACHE STRING "bsf directory")
include(${BSF_DIRECTORY}/Source/CMake/ProjectDefaults.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions( -Wall -O2 -fexceptions )

file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable(main ${SOURCES})
target_include_directories(main PRIVATE "${PROJECT_SOURCE_DIR}/include/")
target_link_libraries(main bsf)

add_engine_dependencies(main)
add_dependencies(main bsfFBXImporter bsfFontImporter bsfFreeImgImporter)

copyBsfBinaries(main ${BSF_DIRECTORY})

# TinySoundFont
find_package(TinySoundFont REQUIRED)
target_include_directories(main PRIVATE ${TSF_INCLUDE_DIR})

# OpenVR
find_package(OpenVR REQUIRED)
target_include_directories(main PRIVATE ${OPENVR_INCLUDE_DIR})
target_link_libraries(main ${OPENVR_LIBRARY})