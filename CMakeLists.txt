cmake_policy(SET CMP0048 NEW)
project(flyMS LANGUAGES CXX C VERSION 0.1.0)
cmake_minimum_required(VERSION 3.5)

# Add cmake subdirectory to the CMAKE_MODULE_PATH so that we can find dependencies
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Set options
option(CROSS_COMPILE "Cross compile" ON)
option(FLYMS_BUILD_TESTS "Build tests" OFF)
option(CODE_COVERAGE "Build unit tests with code coverage" OFF)
option(BUILD_DOCS "Build documentation" ON)

# # Get the architecture of the CPU building this
EXECUTE_PROCESS(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE)
string(COMPARE EQUAL "${ARCHITECTURE}" x86_64 host64bit)

if(host64bit AND CROSS_COMPILE)
  message(STATUS "Cross compiling for 32 bit ARMv7")
  # Set the location of the toolchain file if needed
  set(CMAKE_TOOLCHAIN_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain.cmake"
    CACHE FILEPATH
    "toolchain path" FORCE)

  message(STATUS "Cross compiling to ARM   ${CMAKE_TOOLCHAIN_FILE}")
elseif (host64bit AND NOT CROSS_COMPILE)
  message(STATUS "Building for x86_64")
endif()


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set c++ standard
set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -Wall") # Uncomment to profile with gprof
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-O2 -g")

# Set the binary and library directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Settings for Unit Tests
if (FLYMS_BUILD_TESTS)
  enable_testing()
  include(cmake/unit_test_helpers.cmake)

  message(STATUS "Overriding build type for code coverage")
  set(CMAKE_BUILD_TYPE DEBUG)
  set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -ftest-coverage -fprofile-arcs")

  # These commands will have no effect if the CODE_COVERAGE variable is not set to ON
  include(cmake/code_coverage.cmake)
  add_code_coverage_all_targets(EXCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/external* *test* /usr*)
endif()

add_subdirectory(external)

# Build the Digital Filter Library
add_subdirectory(lib)

add_subdirectory(app)

if (BUILD_DOCS)
  add_subdirectory(docs)
endif()