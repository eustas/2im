cmake_minimum_required(VERSION 3.10)
project(c)

set(CMAKE_CXX_STANDARD 11)

include(CheckCXXSourceCompiles)
check_cxx_source_compiles(
   "int main() {
      #if !defined(__EMSCRIPTEN__)
      static_assert(false, \"__EMSCRIPTEN__ is not defined\");
      #endif
      return 0;
    }"
  TWIM_EMSCRIPTEN
)

message(STATUS "CMAKE_SYSTEM_PROCESSOR is ${CMAKE_SYSTEM_PROCESSOR}")

find_package(ZLIB REQUIRED)  # dependency of PNG
find_package(PNG REQUIRED)

option(BUILD_TESTING "" OFF)
include(CTest)
add_subdirectory(../third_party/highway third_party/highway)

add_library(twimBase STATIC
  codec_params.cc
  codec_params.h
  distance_range.cc
  distance_range.h
  image.cc
  image.h
  platform.h
  xrange_code.h
  region.cc
  region.h
  sin_cos.cc
  sin_cos.h)
target_include_directories(twimBase PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(twimDecoder STATIC
  decoder.cc
  decoder.h
  xrange_decoder.cc
  xrange_decoder.h)
target_include_directories(twimDecoder PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(twimDecoder PUBLIC twimBase)

add_library(twimEncoder STATIC
  encoder.cc
  encoder_internal.h
  encoder_simd.cc
  encoder_simd.h
  encoder.h
  xrange_encoder.cc
  xrange_encoder.h)
target_include_directories(twimEncoder PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(twimEncoder PUBLIC "hwy" twimBase)

add_library(twimIo STATIC
  io.cc
  io.h
)
target_include_directories(twimIo PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${PNG_INCLUDE_DIRS}")
target_link_libraries(twimIo PUBLIC "${PNG_LIBRARIES}" twimBase)

if (NOT "${TWIM_EMSCRIPTEN}")
  add_executable(twim main.cc)
  target_include_directories(twim PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(twim PUBLIC twimDecoder twimEncoder twimIo)
endif()

if ("${TWIM_EMSCRIPTEN}")
  set(WASM_BASE_FLAGS "\
    -O3 \
    --closure 1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -flto \
    --llvm-lto 1 \
    -s DISABLE_EXCEPTION_CATCHING=1 \
  ")
  set(WASM_LINK_FLAGS "\
    ${WASM_BASE_FLAGS} \
    -s MODULARIZE=1 \
    -s FILESYSTEM=0 \
  ")

  add_executable(twimEncoderModule encoder_wasm.cc)
  target_include_directories(twimEncoderModule PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(twimEncoderModule PUBLIC twimBase twimEncoder)

  set_target_properties(twimEncoderModule PROPERTIES LINK_FLAGS "\
    ${WASM_LINK_FLAGS} \
    -s EXPORT_NAME=\"TwimEncoderModule\" \
    -s EXPORTED_FUNCTIONS='[\"_malloc\",\"_free\", \"_twimEncode\"]' \
  ")
endif()
