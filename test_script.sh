#!/bin/bash
# Rebuild and run tests
cd build || exit 125 # Skip if build dir missing
make clean > /dev/null 2>&1
cmake .. > /dev/null 2>&1
make -j4 > /dev/null 2>&1 || exit 125 # Skip if compile fails
cd tests
./run_tests.sh > /dev/null 2>&1
exit $?
