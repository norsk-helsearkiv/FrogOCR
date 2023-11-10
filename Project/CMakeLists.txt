cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME "FrogOCR")

set(CMAKE_C_COMPILER "/usr/bin/gcc-12")
set(CMAKE_CXX_COMPILER "/usr/bin/g++-12")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_INSTALL_RPATH ".")
set(CMAKE_BUILD_WITH_INSTALL_RPATH true)

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
        "${THIRDPARTY_RELEASE_DIR}/paddle/build/paddle_inference_install_dir/paddle/include"
        "${THIRDPARTY_RELEASE_DIR}/opencv/build/install-include"
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
        "${THIRDPARTY_RELEASE_DIR}/tesseract/bin/release/.libs/libtesseract.a"
        "${THIRDPARTY_RELEASE_DIR}/leptonica/build/src/libleptonica.a"
        "${THIRDPARTY_RELEASE_DIR}/libxml2/.libs/libxml2.a"
        "${THIRDPARTY_RELEASE_DIR}/postgresql/src/interfaces/libpq/libpq.a"
        "${THIRDPARTY_RELEASE_DIR}/postgresql/src/port/libpgport.a"
        "${THIRDPARTY_RELEASE_DIR}/postgresql/src/common/libpgcommon.a"
        "${THIRDPARTY_RELEASE_DIR}/fmtlib/build/libfmt.a"
        "${THIRDPARTY_RELEASE_DIR}/paddle/build/paddle_inference_install_dir/paddle/lib/libpaddle_inference.a"
        "${THIRDPARTY_RELEASE_DIR}/paddle/build/paddle_inference_install_dir/paddle/lib/libpir.a"
        "${THIRDPARTY_RELEASE_DIR}/paddle/build/paddle_inference_install_dir/third_party/install/protobuf/lib/libprotobuf.a"
        "${THIRDPARTY_RELEASE_DIR}/paddle/build/paddle_inference_install_dir/third_party/install/glog/lib/libglog.a"
        "${THIRDPARTY_RELEASE_DIR}/paddle/build/paddle_inference_install_dir/third_party/install/gflags/lib/libgflags.a"
        "${THIRDPARTY_RELEASE_DIR}/paddle/build/paddle_inference_install_dir/third_party/install/xxhash/lib/libxxhash.a"
        "${THIRDPARTY_RELEASE_DIR}/paddle/build/paddle_inference_install_dir/third_party/install/cryptopp/lib/libcryptopp.a"
        "${THIRDPARTY_RELEASE_DIR}/paddle/build/paddle_inference_install_dir/third_party/install/utf8proc/lib/libutf8proc.a"
        "${THIRDPARTY_RELEASE_DIR}/paddle/build/libmkldnn.a"
        "${THIRDPARTY_RELEASE_DIR}/paddle/build/paddle_inference_install_dir/third_party/install/mkldnn/lib/libdnnl.so.3"
        "${THIRDPARTY_RELEASE_DIR}/paddle/build/paddle_inference_install_dir/third_party/install/mklml/lib/libiomp5.so"
        "${THIRDPARTY_RELEASE_DIR}/paddle/build/paddle_inference_install_dir/third_party/install/mklml/lib/libmklml_intel.so"
        "${THIRDPARTY_RELEASE_DIR}/paddle/build/paddle/phi/libphi.a"
        "${THIRDPARTY_RELEASE_DIR}/opencv/build/lib/libopencv_imgcodecs.a"
        "${THIRDPARTY_RELEASE_DIR}/opencv/build/lib/libopencv_imgproc.a"
        "${THIRDPARTY_RELEASE_DIR}/opencv/build/lib/libopencv_core.a" # Link order: core must be last.
        "${THIRDPARTY_RELEASE_DIR}/opencv/build/3rdparty/lib/libade.a"
        "${THIRDPARTY_RELEASE_DIR}/opencv/build/3rdparty/lib/libIlmImf.a"
        "${THIRDPARTY_RELEASE_DIR}/opencv/build/3rdparty/lib/libittnotify.a"
        "${THIRDPARTY_RELEASE_DIR}/opencv/build/3rdparty/lib/liblibjasper.a"
        "${THIRDPARTY_RELEASE_DIR}/opencv/build/3rdparty/lib/liblibprotobuf.a"
        libjpeg.so.8
        libopenjp2.so
        libpng.so
        libtiff.so
        liblzma.so
        libz.so
        libcrypto.so.3
        libssl.so.3
        smbclient
        )

target_link_libraries(${PROJECT_NAME} ${ALL_LINK_LIBRARIES})
