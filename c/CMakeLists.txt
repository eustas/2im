cmake_minimum_required(VERSION 3.14)
project(c)

set(CMAKE_CXX_STANDARD 11)

find_package(ZLIB REQUIRED)  # dependency of PNG
find_package(PNG REQUIRED)

add_library(twimLib STATIC
        codec_params.cc
        codec_params.h
        crc64.h
        decoder.cc
        decoder.h
        distance_range.cc
        distance_range.h
        encoder.cc
        encoder.h
        encoder_internal.h
        image.cc
        image.h
        io.cc
        io.h
        platform.h
        range_code.h
        range_decoder.cc
        range_decoder.h
        range_encoder.cc
        range_encoder.h
        region.cc
        region.h
        sin_cos.cc
        sin_cos.h)

target_link_libraries(twimLib PUBLIC
        "${PNG_LIBRARIES}")

target_include_directories(twimLib PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PNG_INCLUDE_DIRS}")

add_executable(twim main.cc)

target_include_directories(twim PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(twim PUBLIC
        twimLib)
