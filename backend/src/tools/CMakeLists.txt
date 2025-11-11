

project(tools)

# Phase 7: Changed from INTERFACE to static library to support rt_utils.cpp
# Phase 8: Added packet_ring.cpp for TPACKET_V3 support
# Phase 11: Link with platform library for compat.hpp
# Phase 12: Added logger.cpp and metrics.cpp for observability
add_library(${PROJECT_NAME} STATIC
    src/rt_utils.cpp
    src/packet_ring.cpp
    src/logger.cpp
    src/metrics.cpp
)

target_include_directories( ${PROJECT_NAME}
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/include
)

# Phase 11: Link with platform library
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        platform
)