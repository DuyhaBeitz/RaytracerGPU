cmake_minimum_required(VERSION 3.20)

project(RTX_GPU LANGUAGES CXX VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)

find_package(raylib 5.0 REQUIRED)

include_directories(${raylib_INCLUDE_DIR})

set(BUILD_OUTPUT_DIR ${CMAKE_BINARY_DIR}/${PROJECT_NAME})

file(MAKE_DIRECTORY ${BUILD_OUTPUT_DIR})

file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

include_directories("${CMAKE_SOURCE_DIR}/src") 

target_link_libraries(${PROJECT_NAME} PRIVATE ${raylib_LIBRARY})

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BUILD_OUTPUT_DIR}
)