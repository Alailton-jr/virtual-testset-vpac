# CMake generated Testfile for 
# Source directory: /Users/alailtonjr/Github/Virtual-TestSet/backend
# Build directory: /Users/alailtonjr/Github/Virtual-TestSet/backend
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[basic_help_test]=] "Virtual-TestSet" "--help")
set_tests_properties([=[basic_help_test]=] PROPERTIES  WILL_FAIL "TRUE" WORKING_DIRECTORY "/Users/alailtonjr/Github/Virtual-TestSet/backend" _BACKTRACE_TRIPLES "/Users/alailtonjr/Github/Virtual-TestSet/backend/CMakeLists.txt;100;add_test;/Users/alailtonjr/Github/Virtual-TestSet/backend/CMakeLists.txt;0;")
subdirs("src/platform")
subdirs("src/main")
subdirs("src/protocols")
subdirs("src/tools")
subdirs("src/sampledValue")
subdirs("src/tests")
subdirs("src/sniffer")
subdirs("src/goose")
subdirs("src/api")
subdirs("src/core")
subdirs("src/synth")
subdirs("src/io")
subdirs("src/sequence")
subdirs("src/analyzer")
subdirs("src/testers")
subdirs("tests")
