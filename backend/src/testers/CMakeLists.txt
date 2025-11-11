add_library(vts_testers
    src/impedance_calculator.cpp
    src/ramping_tester.cpp
    src/distance_tester.cpp
    src/overcurrent_tester.cpp
    src/differential_tester.cpp
)

target_include_directories(vts_testers PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(vts_testers PUBLIC
    vts_core
    tools
)
