# Phase 11: Platform compatibility layer
# This directory provides platform-specific headers and network implementations

# Check platform and add platform-specific implementations
if(APPLE)
    # On macOS, compile BPF implementation
    add_library(platform STATIC
        src/bpf_macos.cpp
    )
    
    target_include_directories(platform
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
elseif(WIN32)
    # On Windows, compile Npcap implementation
    add_library(platform STATIC
        src/npcap_windows.cpp
    )
    
    target_include_directories(platform
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    # Link Npcap/WinPcap libraries
    target_link_libraries(platform
        PUBLIC
            wpcap
            iphlpapi
            ws2_32
    )
else()
    # On Linux, header-only interface library
    add_library(platform INTERFACE)
    
    target_include_directories(platform
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
endif()
