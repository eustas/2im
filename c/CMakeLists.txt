cmake_minimum_required(VERSION 3.14)
project(c)

set(CMAKE_CXX_STANDARD 11)

find_package(ZLIB REQUIRED)  # dependency of PNG
find_package(PNG REQUIRED)

option(BUILD_TESTING "" OFF)
include(CTest)
add_subdirectory(../third_party/highway third_party/highway)

add_library(twimLib STATIC
        codec_params.cc
        codec_params.h
        crc64.h
        decoder.cc
        decoder.h
        distance_range.cc
        distance_range.h
        encoder.cc
        encoder_internal.h
        encoder_simd.cc
        encoder_simd.h
        encoder.h
        image.cc
        image.h
        io.cc
        io.h
        platform.h
        xrange_code.h
        xrange_decoder.cc
        xrange_decoder.h
        xrange_encoder.cc
        xrange_encoder.h
        region.cc
        region.h
        sin_cos.cc
        sin_cos.h)

target_link_libraries(twimLib PUBLIC
        "hwy"
        "${PNG_LIBRARIES}")

target_include_directories(twimLib PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PNG_INCLUDE_DIRS}")

add_executable(twim main.cc)

target_include_directories(twim PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(twim PUBLIC
        twimLib)
