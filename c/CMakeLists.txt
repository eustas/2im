cmake_minimum_required(VERSION 3.10)
project(c)

set(CMAKE_CXX_STANDARD 11)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msimd128")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -flto -fno-exceptions -fno-rtti")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -fno-exceptions -fno-rtti -g2")

include(CheckCXXSourceCompiles)
check_cxx_source_compiles(
   "int main() {
      #if !defined(__wasm32__)
      static_assert(false, \"__wasm32__ is not defined\");
      #endif
      return 0;
    }"
  TWIM_WASM
)
check_cxx_source_compiles(
   "int main() {
      #if !defined(__wasm_simd128__)
      static_assert(false, \"__wasm_simd128__ is not defined\");
      #endif
      return 0;
    }"
  TWIM_WASM_SIMD
)

if (NOT "${TWIM_WASM}")
  find_package(ZLIB REQUIRED)  # dependency of PNG
  find_package(PNG REQUIRED)
endif()

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
  sin_cos.h
)
target_include_directories(twimBase PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(twimDecoder STATIC
  decoder.cc
  decoder.h
  xrange_decoder.cc
  xrange_decoder.h
)
target_include_directories(twimDecoder PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(twimDecoder PUBLIC twimBase)

add_library(twimEncoder STATIC
  encoder.cc
  encoder_internal.h
  encoder_simd.cc
  encoder_simd.h
  encoder.h
  xrange_encoder.cc
  xrange_encoder.h
)
target_include_directories(twimEncoder PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" ../third_party/highway)
target_link_libraries(twimEncoder PUBLIC twimBase)

add_library(simpleAlloc STATIC
  simple_alloc.cc
)

if (NOT "${TWIM_WASM}")
  add_library(twimIo STATIC
    io.cc
    io.h
  )
  target_include_directories(twimIo PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${PNG_INCLUDE_DIRS}")
  target_link_libraries(twimIo PUBLIC "${PNG_LIBRARIES}" twimBase)

  add_executable(twim main.cc)
  target_include_directories(twim PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(twim PUBLIC twimDecoder twimEncoder twimIo)
endif()

if ("${TWIM_WASM}")
  set(WASM_FLAGS "\
    -s ALLOW_MEMORY_GROWTH=1 \
    -s DISABLE_EXCEPTION_CATCHING=1 \
    -s STANDALONE_WASM=1 \
    -s TOTAL_MEMORY=128kb \
    -s TOTAL_STACK=64kb \
    -s FILESYSTEM=0 \
  ")

  if ("${TWIM_WASM_SIMD}")
    set(TWIM_MODULE "simd-twim")
  else()
    set(TWIM_MODULE "twim")
  endif()

  add_executable("${TWIM_MODULE}" encoder_wasm.cc)
  target_include_directories("${TWIM_MODULE}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries("${TWIM_MODULE}" PUBLIC twimEncoder simpleAlloc)

  set_target_properties("${TWIM_MODULE}" PROPERTIES LINK_FLAGS "\
    ${WASM_FLAGS} \
    -s EXPORT_NAME=\"TwimEncoderModule\" \
    -s EXPORTED_FUNCTIONS='[\"_malloc\",\"_free\", \"_twimEncode\"]' \
  ")
endif()
