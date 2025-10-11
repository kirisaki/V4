cmake_minimum_required(VERSION 3.16)
project(V4VM LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Libraries
add_library(v4vm STATIC
    src/vm_core.cpp
)
target_include_directories(v4vm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_options(v4vm PRIVATE
    -Wall -Wextra -fno-exceptions -fno-rtti -Os
)

# ---- Tools
add_executable(v4repl tools/v4repl.cpp)
target_link_libraries(v4repl PRIVATE v4vm)

# ---- Tests----
option(V4_BUILD_TESTS "Build unit tests" ON)

if (V4_BUILD_TESTS)
  enable_testing()

  # C++ Tests
  add_executable(test_vm tests/test_vm.cpp)
  target_link_libraries(test_vm PRIVATE v4vm)
  target_include_directories(test_vm PRIVATE tests/vendor/doctest)
  add_test(NAME test_vm COMMAND test_vm)

  # C Tests
  add_executable(test_vm_c tests/test_vm_c.c)
  target_include_directories(test_vm_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(test_vm_c PRIVATE v4vm)
  if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_vm_c PRIVATE -Wall -Wextra -Werror -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wconversion)
  endif()
  add_test(NAME test_vm_c COMMAND test_vm_c)
  
endif()