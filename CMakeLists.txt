cmake_minimum_required(VERSION 3.16)
project(V4VM LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Libraries
add_library(v4vm STATIC src/core.cpp src/memory.cpp)
target_include_directories(v4vm PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(v4vm PRIVATE ${PROJECT_SOURCE_DIR}/include/v4/internal)
target_compile_options(v4vm PRIVATE -Wall -Wextra -fno-exceptions -fno-rtti -Os)

# ---- Tools
add_executable(v4repl tools/v4repl.cpp)
target_link_libraries(v4repl PRIVATE v4vm)

# ---- Tests
option(V4_BUILD_TESTS "Build unit tests" ON)

if(V4_BUILD_TESTS)
  enable_testing()

  # C++ Tests VM tests
  add_executable(test_vm tests/test_vm.cpp)
  target_link_libraries(test_vm PRIVATE v4vm)
  target_include_directories(test_vm PRIVATE tests/vendor/doctest)
  add_test(NAME test_vm COMMAND test_vm)

  # Memory tests
  add_executable(test_memory tests/test_memory.cpp)
  target_link_libraries(test_memory PRIVATE v4vm)
  target_include_directories(test_memory PRIVATE tests/vendor/doctest)
  add_test(NAME test_memory COMMAND test_memory)

  # C Tests
  add_executable(test_vm_c tests/test_vm_c.c)
  target_include_directories(test_vm_c PRIVATE ${PROJECT_SOURCE_DIR}/include)
  target_link_libraries(test_vm_c PRIVATE v4vm)
  set_target_properties(test_vm_c PROPERTIES LINKER_LANGUAGE CXX)

  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
      test_vm_c
      PRIVATE -Wall
              -Wextra
              -Werror
              -Wstrict-prototypes
              -Wmissing-prototypes
              -Wshadow
              -Wconversion)
  endif()
  add_test(NAME test_vm_c COMMAND test_vm_c)

endif()
