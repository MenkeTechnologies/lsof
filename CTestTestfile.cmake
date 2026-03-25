# CMake generated Testfile for 
# Source directory: /Users/wizard/forkedRepos/lsof
# Build directory: /Users/wizard/forkedRepos/lsof
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(unit_tests "/Users/wizard/forkedRepos/lsof/test_unit")
set_tests_properties(unit_tests PROPERTIES  _BACKTRACE_TRIPLES "/Users/wizard/forkedRepos/lsof/CMakeLists.txt;88;add_test;/Users/wizard/forkedRepos/lsof/CMakeLists.txt;0;")
add_test(integration_tests "/Users/wizard/forkedRepos/lsof/test_integration")
set_tests_properties(integration_tests PROPERTIES  _BACKTRACE_TRIPLES "/Users/wizard/forkedRepos/lsof/CMakeLists.txt;93;add_test;/Users/wizard/forkedRepos/lsof/CMakeLists.txt;0;")
