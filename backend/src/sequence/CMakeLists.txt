cmake_minimum_required(VERSION 3.10)

project(sequence)

add_library(${PROJECT_NAME}
    src/sequence_engine.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/core/include
        ${CMAKE_SOURCE_DIR}/src/platform/include
        ${CMAKE_SOURCE_DIR}/src/tools/include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        vts_core
        pthread
)
