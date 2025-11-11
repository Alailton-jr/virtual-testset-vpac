cmake_minimum_required(VERSION 3.5)

# IO library - COMTRADE/CSV parser and file I/O utilities
add_library(vts_io
    src/comtrade_parser.cpp
)

target_include_directories(vts_io PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(vts_io PUBLIC
    # No external dependencies needed - uses only standard library
)

# Set C++17 standard
target_compile_features(vts_io PUBLIC cxx_std_17)

# Installation rules
install(TARGETS vts_io
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
)
