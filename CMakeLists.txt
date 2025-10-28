cmake_minimum_required(VERSION 3.16)
project(V4VM LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Libraries
add_library(v4vm STATIC src/core.cpp src/memory.cpp src/arena.cpp)
target_include_directories(v4vm PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(v4vm PRIVATE ${PROJECT_SOURCE_DIR}/include/v4/internal)

# Mock HAL for testing
add_library(mock_hal STATIC tests/mock_hal.cpp)
target_include_directories(mock_hal PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(mock_hal PUBLIC ${PROJECT_SOURCE_DIR}/tests)

if(MSVC)
  target_compile_options(v4vm PRIVATE /W4 /GR- /EHs-c-)
  # Debug: without optimization
  target_compile_options(v4vm PRIVATE $<$<CONFIG:Debug>:/Od>)
  # Release: with optimization
  target_compile_options(v4vm PRIVATE $<$<CONFIG:Release>:/O1>)
else()
  target_compile_options(v4vm PRIVATE -Wall -Wextra -fno-exceptions -fno-rtti -Os)
endif()

# ---- Tools
add_executable(v4repl tools/v4repl.cpp)
target_link_libraries(v4repl PRIVATE v4vm mock_hal)

# ---- Tests
option(V4_BUILD_TESTS "Build unit tests" ON)

if(V4_BUILD_TESTS)
  enable_testing()

  # C++ Tests VM tests
  add_executable(test_vm tests/test_vm.cpp)
  target_link_libraries(test_vm PRIVATE v4vm mock_hal)
  target_include_directories(test_vm PRIVATE tests/vendor/doctest)
  add_test(NAME test_vm COMMAND test_vm)

  add_executable(test_return_stack tests/test_return_stack.cpp)
  target_link_libraries(test_return_stack PRIVATE v4vm mock_hal)
  target_include_directories(test_return_stack PRIVATE tests/vendor/doctest)
  add_test(NAME test_return_stack COMMAND test_return_stack)

  add_executable(test_call tests/test_call.cpp)
  target_link_libraries(test_call PRIVATE v4vm mock_hal)
  target_include_directories(test_call PRIVATE tests/vendor/doctest)
  add_test(NAME test_call COMMAND test_call)

  # Memory tests
  add_executable(test_memory tests/test_memory.cpp)
  target_link_libraries(test_memory PRIVATE v4vm mock_hal)
  target_include_directories(test_memory PRIVATE tests/vendor/doctest)
  add_test(NAME test_memory COMMAND test_memory)

  # Arena tests
  add_executable(test_arena tests/test_arena.cpp)
  target_link_libraries(test_arena PRIVATE v4vm mock_hal)
  target_include_directories(test_arena PRIVATE tests/vendor/doctest)
  add_test(NAME test_arena COMMAND test_arena)

  # SYS instruction tests
  add_executable(test_sys tests/test_sys.cpp)
  target_link_libraries(test_sys PRIVATE mock_hal v4vm)
  target_include_directories(test_sys PRIVATE tests/vendor/doctest)
  add_test(NAME test_sys COMMAND test_sys)

  # C Tests
  add_executable(test_vm_c tests/test_vm_c.c)
  target_include_directories(test_vm_c PRIVATE ${PROJECT_SOURCE_DIR}/include)
  target_link_libraries(test_vm_c PRIVATE v4vm mock_hal)
  set_target_properties(test_vm_c PROPERTIES LINKER_LANGUAGE CXX)
  add_test(NAME test_vm_c COMMAND test_vm_c)

endif()
#
