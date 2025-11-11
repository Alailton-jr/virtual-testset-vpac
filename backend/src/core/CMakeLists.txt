add_library(vts_core
    src/sv_publisher_manager.cpp
    src/sv_publisher_instance.cpp
    src/global_flags.cpp
)

target_include_directories(vts_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/platform/include 
        ${CMAKE_SOURCE_DIR}/third_party/json/include
)

target_link_libraries(vts_core
    PRIVATE
        pthread
)
