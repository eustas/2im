cmake_minimum_required(VERSION 3.11)
project(c)

set(CMAKE_CXX_STANDARD 11)

include(FetchContent)

FetchContent_Declare(
  GTest
  URL https://github.com/google/googletest/archive/refs/tags/release-1.11.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(GTest)

FetchContent_Declare(
  highway
  URL https://github.com/eustas/highway/archive/refs/heads/master.zip
)
FetchContent_MakeAvailable(highway)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msimd128")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -flto -fno-exceptions -fno-rtti")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -flto -fno-exceptions -fno-rtti -g2")

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
  find_package(Threads REQUIRED)
endif()

add_library(twimBase STATIC
  codec_params.cc
  codec_params.h
  distance_range.cc
  distance_range.h
  image.cc
  image.h
  platform.cc
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
target_link_libraries(twimEncoder PUBLIC twimBase hwy)
if (NOT "${TWIM_WASM}")
  target_link_libraries(twimEncoder PRIVATE Threads::Threads)
endif()

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
  set(CMAKE_EXECUTABLE_SUFFIX ".wasm")

  set(WASM_FLAGS "\
    -s ALLOW_MEMORY_GROWTH=1 \
    -s DISABLE_EXCEPTION_CATCHING=1 \
    -s STANDALONE_WASM=1 \
    -s TOTAL_MEMORY=384kb \
    -s TOTAL_STACK=64kb \
    -s FILESYSTEM=0 \
    -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
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

  add_custom_command(TARGET "${TWIM_MODULE}" POST_BUILD
    COMMAND wasm-metadce ARGS --enable-simd $<TARGET_FILE:${TWIM_MODULE}> -o $<TARGET_FILE:${TWIM_MODULE}> -f twim-exports.json
    COMMAND wasm-opt ARGS -Oz --enable-simd $<TARGET_FILE:${TWIM_MODULE}> -o $<TARGET_FILE:${TWIM_MODULE}>
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
endif()

# Testing ----------------------------------------------------------------------

enable_testing()
include(GoogleTest)

set(TWIM_TEST_FILES
  codec_params_test.cc
  crc64_test.cc
  encoder_test.cc
  region_test.cc
  sin_cos_test.cc
  xrange_test.cc
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests)
foreach (TESTFILE IN LISTS TWIM_TEST_FILES)
  # The TESTNAME is the name without the extension or directory.
  get_filename_component(TESTNAME ${TESTFILE} NAME_WE)
  add_executable(${TESTNAME} ${TESTFILE})
  target_link_libraries(${TESTNAME} twimDecoder twimEncoder gtest_main)
  gtest_discover_tests(${TESTNAME})
endforeach()