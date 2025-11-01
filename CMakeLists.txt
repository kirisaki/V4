cmake_minimum_required(VERSION 3.16)
project(
  V4VM
  VERSION 0.8.0
  LANGUAGES C CXX)

# ============================================================================
# Project Configuration
# ============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(V4_BUILD_TESTS "Build unit tests" ON)
option(V4_BUILD_TOOLS "Build command-line tools" ON)
option(V4_ENABLE_MOCK_HAL "Build with mock HAL (required for tests)" ON)
option(V4_USE_V4HAL "Use V4-hal C++17 CRTP implementation" OFF)
option(V4_OPTIMIZE_SIZE "Optimize for size (-Os)" ON)
option(V4_ENABLE_LTO "Enable Link Time Optimization (not compatible with sanitizers)" OFF)

# V4-hal path configuration
set(V4HAL_LOCAL_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/../V4-hal"
    CACHE PATH "Path to local V4-hal source")

# ============================================================================
# Core VM Library
# ============================================================================

set(V4VM_SOURCES src/core.cpp src/memory.cpp src/arena.cpp)

add_library(v4vm STATIC ${V4VM_SOURCES})

target_include_directories(
  v4vm
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE ${PROJECT_SOURCE_DIR}/include/v4/internal)

# Compiler flags
if(MSVC)
  target_compile_options(
    v4vm
    PRIVATE /W4 # Warning level 4
            /GR- # Disable RTTI
            /EHs-c- # Disable exceptions
  )
  # Configuration-specific optimizations
  target_compile_options(v4vm PRIVATE $<$<CONFIG:Debug>:/Od> $<$<CONFIG:Release>:/O1>)
else()
  target_compile_options(
    v4vm
    PRIVATE -Wall # Enable all warnings
            -Wextra # Enable extra warnings
            -fno-exceptions -fno-rtti)
  if(V4_OPTIMIZE_SIZE)
    target_compile_options(v4vm PRIVATE -Os)
  endif()
endif()

# Enable LTO if supported and requested
if(V4_ENABLE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_supported OUTPUT ipo_output)
  if(ipo_supported)
    set_target_properties(v4vm PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    message(STATUS "LTO enabled for v4vm")
  else()
    message(WARNING "LTO not supported: ${ipo_output}")
  endif()
endif()

# ============================================================================
# Mock HAL Library (for testing and development)
# ============================================================================

if(V4_ENABLE_MOCK_HAL)
  add_library(mock_hal STATIC tests/mock_hal.cpp)

  target_include_directories(mock_hal PUBLIC ${PROJECT_SOURCE_DIR}/include
                                             ${PROJECT_SOURCE_DIR}/tests)

  # Export mock_hal as an alias for easier linking
  add_library(V4::mock_hal ALIAS mock_hal)
endif()

# ============================================================================
# V4-hal Integration (C++17 CRTP HAL implementation)
# ============================================================================

if(V4_USE_V4HAL)
  include(FetchContent)

  if(EXISTS "${V4HAL_LOCAL_PATH}/CMakeLists.txt")
    message(STATUS "Using local V4-hal from: ${V4HAL_LOCAL_PATH}")
    add_subdirectory(${V4HAL_LOCAL_PATH} ${CMAKE_BINARY_DIR}/v4hal)
  else()
    message(STATUS "Fetching V4-hal from repository")
    fetchcontent_declare(
      v4hal
      GIT_REPOSITORY https://github.com/V4-project/V4-hal.git
      GIT_TAG v0.1.0)
    fetchcontent_makeavailable(v4hal)
  endif()

  # Create v4_hal_* wrapper for V4-hal's hal_* API
  add_library(v4_hal_wrapper STATIC src/hal_wrapper.cpp)
  target_link_libraries(v4_hal_wrapper PUBLIC v4-hal-lib)
  target_include_directories(v4_hal_wrapper PUBLIC ${PROJECT_SOURCE_DIR}/include)

  message(STATUS "V4-hal C++17 CRTP implementation enabled")
endif()

# ============================================================================
# Tools
# ============================================================================

# NOTE: v4repl tool has been moved to a separate repository V4_BUILD_TOOLS option is kept
# for future tools

# ============================================================================
# Tests
# ============================================================================

if(V4_BUILD_TESTS)
  enable_testing()

  if(NOT V4_ENABLE_MOCK_HAL)
    message(WARNING "Tests require V4_ENABLE_MOCK_HAL=ON. Skipping test targets.")
  else()
    # Common test settings
    set(DOCTEST_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/tests/vendor/doctest)

    # Helper function to add test executables
    function(add_v4_test TEST_NAME TEST_SOURCE)
      add_executable(${TEST_NAME} ${TEST_SOURCE})
      target_include_directories(${TEST_NAME} PRIVATE ${DOCTEST_INCLUDE_DIR})
      target_link_libraries(${TEST_NAME} PRIVATE v4vm mock_hal)
      add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endfunction()

    # VM Core Tests
    add_v4_test(test_vm tests/test_vm.cpp)
    add_v4_test(test_return_stack tests/test_return_stack.cpp)
    add_v4_test(test_call tests/test_call.cpp)

    # Memory Tests
    add_v4_test(test_memory tests/test_memory.cpp)
    add_v4_test(test_arena tests/test_arena.cpp)
    add_v4_test(test_vm_arena tests/test_vm_arena.cpp)

    # HAL/System Tests
    add_v4_test(test_sys tests/test_sys.cpp)

    # API Extension Tests
    add_v4_test(test_api_extensions tests/test_api_extensions.cpp)

    # C API Tests
    add_executable(test_vm_c tests/test_vm_c.c)
    target_include_directories(test_vm_c PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_link_libraries(test_vm_c PRIVATE v4vm mock_hal)
    set_target_properties(test_vm_c PROPERTIES LINKER_LANGUAGE CXX)
    add_test(NAME test_vm_c COMMAND test_vm_c)
  endif()
endif()

# ============================================================================
# Installation (optional)
# ============================================================================

# Install headers
install(
  DIRECTORY include/v4
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp")

# Install library
install(
  TARGETS v4vm
  EXPORT V4VMTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "V4VM Configuration Summary:")
message(STATUS "  Version:              ${PROJECT_VERSION}")
message(STATUS "  Build type:           ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard:         ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build tests:          ${V4_BUILD_TESTS}")
message(STATUS "  Build tools:          ${V4_BUILD_TOOLS}")
message(STATUS "  Enable mock HAL:      ${V4_ENABLE_MOCK_HAL}")
message(STATUS "  Use V4-hal (C++17):   ${V4_USE_V4HAL}")
message(STATUS "  Optimize for size:    ${V4_OPTIMIZE_SIZE}")
message(STATUS "")
