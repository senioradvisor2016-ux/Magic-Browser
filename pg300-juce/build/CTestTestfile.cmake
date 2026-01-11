# CMake generated Testfile for 
# Source directory: /workspace/pg300-juce
# Build directory: /workspace/pg300-juce/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[pg300_acceptance_tests]=] "/workspace/pg300-juce/build/pg300_tests")
set_tests_properties([=[pg300_acceptance_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/workspace/pg300-juce/CMakeLists.txt;91;add_test;/workspace/pg300-juce/CMakeLists.txt;0;")
subdirs("_deps/juce-build")
