# Phase 13: Unit tests using Google Test

cmake_minimum_required(VERSION 3.14)

# Google Test setup
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Disable strict warnings for Google Test (it doesn't compile with -Werror -Wdouble-promotion)
set(CMAKE_CXX_FLAGS_SAVED "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error -Wno-double-promotion")
FetchContent_MakeAvailable(googletest)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_SAVED}")

enable_testing()

# Test executable
add_executable(vts_tests
    test_ber_encoding.cpp
    test_vlan.cpp
    test_mac_parser.cpp
    test_smpCnt_wrap.cpp
    test_comtrade_parser.cpp
    test_trip_rule_evaluator.cpp
    test_sequence_engine.cpp
    test_impedance_calculator.cpp
    test_ramping_tester.cpp
    test_overcurrent_tester.cpp
    network_integration_test.cpp
    # test_distance_tester.cpp - TODO: Create tests for distance tester
    # test_differential_tester.cpp - TODO: Create tests for differential tester
    # test_threadpool.cpp - TODO: Adapt to ThreadPool's submit() API with function arguments
    test_main.cpp
)

target_link_libraries(vts_tests
    gtest_main
    gmock
    protocols
    tools
    platform
    vts_io
    sniffer
    sequence
    goose
    vts_testers
)

target_include_directories(vts_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/protocols/include
    ${CMAKE_SOURCE_DIR}/src/tools/include
    ${CMAKE_SOURCE_DIR}/src/platform/include
    ${CMAKE_SOURCE_DIR}/src/io/include
    ${CMAKE_SOURCE_DIR}/src/sniffer/include
    ${CMAKE_SOURCE_DIR}/src/sequence/include
    ${CMAKE_SOURCE_DIR}/src/goose/include
    ${CMAKE_SOURCE_DIR}/src/core/include
    ${CMAKE_SOURCE_DIR}/src/testers/include
)

# Discover and register tests
include(GoogleTest)
gtest_discover_tests(vts_tests)

# Individual test targets for convenience
add_test(NAME BER_Encoding COMMAND vts_tests --gtest_filter=BEREncodingTest.*)
add_test(NAME VLAN_Validation COMMAND vts_tests --gtest_filter=VLANTest.*)
add_test(NAME MAC_Parser COMMAND vts_tests --gtest_filter=MACParserTest.*)
add_test(NAME smpCnt_Wrap COMMAND vts_tests --gtest_filter=SmpCntWrapTest.*)
add_test(NAME ThreadPool COMMAND vts_tests --gtest_filter=ThreadPoolTest.*)
add_test(NAME TripRuleEvaluator COMMAND vts_tests --gtest_filter=TripRuleEvaluatorTest.*)
add_test(NAME NetworkIntegration COMMAND vts_tests --gtest_filter=NetworkIntegrationTest.*)
