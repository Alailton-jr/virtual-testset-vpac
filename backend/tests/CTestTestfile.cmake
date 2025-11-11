# CMake generated Testfile for 
# Source directory: /Users/alailtonjr/Github/Virtual-TestSet/backend/tests
# Build directory: /Users/alailtonjr/Github/Virtual-TestSet/backend/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
include("/Users/alailtonjr/Github/Virtual-TestSet/backend/tests/vts_tests[1]_include.cmake")
add_test(BER_Encoding "/Users/alailtonjr/Github/Virtual-TestSet/backend/build/vts_tests" "--gtest_filter=BEREncodingTest.*")
set_tests_properties(BER_Encoding PROPERTIES  _BACKTRACE_TRIPLES "/Users/alailtonjr/Github/Virtual-TestSet/backend/tests/CMakeLists.txt;72;add_test;/Users/alailtonjr/Github/Virtual-TestSet/backend/tests/CMakeLists.txt;0;")
add_test(VLAN_Validation "/Users/alailtonjr/Github/Virtual-TestSet/backend/build/vts_tests" "--gtest_filter=VLANTest.*")
set_tests_properties(VLAN_Validation PROPERTIES  _BACKTRACE_TRIPLES "/Users/alailtonjr/Github/Virtual-TestSet/backend/tests/CMakeLists.txt;73;add_test;/Users/alailtonjr/Github/Virtual-TestSet/backend/tests/CMakeLists.txt;0;")
add_test(MAC_Parser "/Users/alailtonjr/Github/Virtual-TestSet/backend/build/vts_tests" "--gtest_filter=MACParserTest.*")
set_tests_properties(MAC_Parser PROPERTIES  _BACKTRACE_TRIPLES "/Users/alailtonjr/Github/Virtual-TestSet/backend/tests/CMakeLists.txt;74;add_test;/Users/alailtonjr/Github/Virtual-TestSet/backend/tests/CMakeLists.txt;0;")
add_test(smpCnt_Wrap "/Users/alailtonjr/Github/Virtual-TestSet/backend/build/vts_tests" "--gtest_filter=SmpCntWrapTest.*")
set_tests_properties(smpCnt_Wrap PROPERTIES  _BACKTRACE_TRIPLES "/Users/alailtonjr/Github/Virtual-TestSet/backend/tests/CMakeLists.txt;75;add_test;/Users/alailtonjr/Github/Virtual-TestSet/backend/tests/CMakeLists.txt;0;")
add_test(ThreadPool "/Users/alailtonjr/Github/Virtual-TestSet/backend/build/vts_tests" "--gtest_filter=ThreadPoolTest.*")
set_tests_properties(ThreadPool PROPERTIES  _BACKTRACE_TRIPLES "/Users/alailtonjr/Github/Virtual-TestSet/backend/tests/CMakeLists.txt;76;add_test;/Users/alailtonjr/Github/Virtual-TestSet/backend/tests/CMakeLists.txt;0;")
add_test(TripRuleEvaluator "/Users/alailtonjr/Github/Virtual-TestSet/backend/build/vts_tests" "--gtest_filter=TripRuleEvaluatorTest.*")
set_tests_properties(TripRuleEvaluator PROPERTIES  _BACKTRACE_TRIPLES "/Users/alailtonjr/Github/Virtual-TestSet/backend/tests/CMakeLists.txt;77;add_test;/Users/alailtonjr/Github/Virtual-TestSet/backend/tests/CMakeLists.txt;0;")
add_test(NetworkIntegration "/Users/alailtonjr/Github/Virtual-TestSet/backend/build/vts_tests" "--gtest_filter=NetworkIntegrationTest.*")
set_tests_properties(NetworkIntegration PROPERTIES  _BACKTRACE_TRIPLES "/Users/alailtonjr/Github/Virtual-TestSet/backend/tests/CMakeLists.txt;78;add_test;/Users/alailtonjr/Github/Virtual-TestSet/backend/tests/CMakeLists.txt;0;")
subdirs("../_deps/googletest-build")
