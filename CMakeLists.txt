cmake_minimum_required(VERSION 3.10)
project(Task2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

# Определяем компилятор
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(COMPILER_NAME "g++")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(COMPILER_NAME "clang++")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "NVHPC")
    set(COMPILER_NAME "nvc++")
else()
    message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# Опции компиляции
if(NOT CMAKE_CXX_FLAGS)
    set(CMAKE_CXX_FLAGS -O2)
endif()

find_package(Threads REQUIRED)

# Создаём папку для бинарников
set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/${COMPILER_NAME})
file(MAKE_DIRECTORY ${OUTPUT_DIR})

# Основной исполняемый файл
add_executable(task_server main.cpp)
target_link_libraries(task_server PRIVATE Threads::Threads)
set_target_properties(task_server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
target_compile_features(task_server PRIVATE cxx_std_20)

# Тестовый исполняемый файл
add_executable(test_results test_results.cpp)
set_target_properties(test_results PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
target_compile_features(test_results PRIVATE cxx_std_20)
