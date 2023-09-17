cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME "FrogOCR")

set(CMAKE_C_COMPILER "/usr/bin/gcc-12")
set(CMAKE_CXX_COMPILER "/usr/bin/g++-12")
set(CMAKE_CXX_STANDARD 23)

project(${PROJECT_NAME})

set(ROOT_DIR "${PROJECT_SOURCE_DIR}/..")
set(THIRDPARTY_DIR "${ROOT_DIR}/thirdparty")
set(THIRDPARTY_DEBUG_DIR "${THIRDPARTY_DIR}/linux-debug")
set(THIRDPARTY_RELEASE_DIR "${THIRDPARTY_DIR}/linux-release")

include_directories(
        "${ROOT_DIR}/Source"
        "${THIRDPARTY_RELEASE_DIR}/tesseract/include"
        "${THIRDPARTY_RELEASE_DIR}/tesseract/bin/release/include"
        "${THIRDPARTY_RELEASE_DIR}/fmtlib/include"
        "${THIRDPARTY_RELEASE_DIR}/libxml2/include"
        "${THIRDPARTY_RELEASE_DIR}/postgresql/src/include"
        "${THIRDPARTY_RELEASE_DIR}/postgresql/src/interfaces"
        "${THIRDPARTY_RELEASE_DIR}/leptonica/src"
)

if (CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES Debug Release)
    set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "Reset configurations" FORCE)
endif ()

file(GLOB_RECURSE SOURCE_CPP_FILES "${ROOT_DIR}/Source/*.cpp")
file(GLOB_RECURSE SOURCE_HPP_FILES "${ROOT_DIR}/Source/*.hpp")

source_group(TREE ${ROOT_DIR} FILES ${SOURCE_CPP_FILES})
source_group(TREE ${ROOT_DIR} FILES ${SOURCE_HPP_FILES})

add_executable(${PROJECT_NAME} ${SOURCE_CPP_FILES} ${SOURCE_HPP_FILES})

set(ALL_LINK_LIBRARIES
        general "${THIRDPARTY_RELEASE_DIR}/tesseract/bin/release/.libs/libtesseract.a"
        general "${THIRDPARTY_RELEASE_DIR}/leptonica/build/src/libleptonica.a"
        general "${THIRDPARTY_RELEASE_DIR}/libxml2/.libs/libxml2.a"
        general "${THIRDPARTY_RELEASE_DIR}/postgresql/src/interfaces/libpq/libpq.a"
        general "${THIRDPARTY_RELEASE_DIR}/postgresql/src/port/libpgport.a"
        general "${THIRDPARTY_RELEASE_DIR}/postgresql/src/common/libpgcommon.a"
        general "${THIRDPARTY_RELEASE_DIR}/fmtlib/build/libfmt.a"
        general libjpeg.so.8
        general libopenjp2.so
        general libpng.so
        general libtiff.so
        general liblzma.so
        general libz.so
        general libcrypto.so.3
        general libssl.so.3
        )

target_link_libraries(${PROJECT_NAME} ${ALL_LINK_LIBRARIES})
