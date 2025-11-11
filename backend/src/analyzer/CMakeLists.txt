# Analyzer Module
add_library(vts_analyzer
    src/analyzer_engine.cpp
)

target_include_directories(vts_analyzer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/tools/include
)

target_link_libraries(vts_analyzer
    PUBLIC
        tools
        Threads::Threads
)

# Set C++ standard
target_compile_features(vts_analyzer PUBLIC cxx_std_17)
