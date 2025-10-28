cmake_minimum_required(VERSION 3.16)
project(V4VM VERSION 0.2.1 LANGUAGES C CXX)

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
option(V4_OPTIMIZE_SIZE "Optimize for size (-Os)" ON)

# ============================================================================
# Core VM Library
# ============================================================================

set(V4VM_SOURCES
  src/core.cpp
  src/memory.cpp
  src/arena.cpp
)

add_library(v4vm STATIC ${V4VM_SOURCES})

target_include_directories(v4vm
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include/v4/internal
)

# Compiler flags
if(MSVC)
  target_compile_options(v4vm PRIVATE
    /W4           # Warning level 4
    /GR-          # Disable RTTI
    /EHs-c-       # Disable exceptions
  )
  # Configuration-specific optimizations
  target_compile_options(v4vm PRIVATE
    $<$<CONFIG:Debug>:/Od>
    $<$<CONFIG:Release>:/O1>
  )
else()
  target_compile_options(v4vm PRIVATE
    -Wall         # Enable all warnings
    -Wextra       # Enable extra warnings
    -fno-exceptions
    -fno-rtti
  )
  if(V4_OPTIMIZE_SIZE)
    target_compile_options(v4vm PRIVATE -Os)
  endif()
endif()

# ============================================================================
# Mock HAL Library (for testing and development)
# ============================================================================

if(V4_ENABLE_MOCK_HAL)
  add_library(mock_hal STATIC tests/mock_hal.cpp)

  target_include_directories(mock_hal
    PUBLIC
      ${PROJECT_SOURCE_DIR}/include
      ${PROJECT_SOURCE_DIR}/tests
  )

  # Export mock_hal as an alias for easier linking
  add_library(V4::mock_hal ALIAS mock_hal)
endif()

# ============================================================================
# Tools
# ============================================================================

if(V4_BUILD_TOOLS AND V4_ENABLE_MOCK_HAL)
  add_executable(v4repl tools/v4repl.cpp)
  target_link_libraries(v4repl PRIVATE v4vm mock_hal)
endif()

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
install(DIRECTORY include/v4
  DESTINATION include
  FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Install library
install(TARGETS v4vm
  EXPORT V4VMTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

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
message(STATUS "  Optimize for size:    ${V4_OPTIMIZE_SIZE}")
message(STATUS "")
