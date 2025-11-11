add_library(vts_synth
    src/phasor_synth.cpp
)

target_include_directories(vts_synth
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
