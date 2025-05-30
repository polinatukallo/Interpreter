cmake_minimum_required(VERSION 3.12)

project(
    itmoscript_interpreter
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)

include_directories(lib)
add_subdirectory(lib)
add_subdirectory(bin)
#target_compile_options(itmoscript_interpreter PRIVATE "-frtti")

enable_testing()
add_subdirectory(tests)