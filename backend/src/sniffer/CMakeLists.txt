

project(sniffer)

add_library(${PROJECT_NAME} 
    src/sniffer.cpp
    src/trip_rule_evaluator.cpp
)

target_include_directories( ${PROJECT_NAME}
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/core/include
        ${CMAKE_SOURCE_DIR}/src/analyzer/include
)

target_link_libraries(${PROJECT_NAME}
    tools
    vts_analyzer
    ${FFTW3_LIBRARY}
)