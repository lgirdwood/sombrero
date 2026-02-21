#!/bin/bash

# run_tests.sh
# A high-level script to invoke all libsmbrr unit tests and report results.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
TESTS_DIR="$SCRIPT_DIR"
IMAGE="${TESTS_DIR}/wiz-ha-x.bmp"

if [ ! -d "$TESTS_DIR" ]; then
    echo "Error: Tests directory $TESTS_DIR not found. Please build the project first."
    exit 1
fi

if [ ! -f "$IMAGE" ]; then
    echo "Error: Test image $IMAGE not found."
    exit 1
fi

TOTAL=0
PASSED=0
FAILED=0

run_test() {
    local test_name="$1"
    shift
    local cmd=("./$test_name" "$@")
    
    echo "============================================================"
    echo "Running Test: $test_name"
    echo "Command: ${cmd[*]}"
    echo "------------------------------------------------------------"
    
    # Run the command and let stdout/stderr pass through
    "${cmd[@]}"
    local status=$?
    
    echo "------------------------------------------------------------"
    if [ $status -eq 0 ]; then
        echo -e "\033[0;32mRESULT: [ PASS ] $test_name\033[0m"
        ((PASSED++))
    else
        echo -e "\033[0;31mRESULT: [ FAIL ] $test_name (Exit code: $status)\033[0m"
        ((FAILED++))
    fi
    ((TOTAL++))
    echo "============================================================"
    echo ""
}

# Change to TESTS_DIR so output files are written there (keeps root clean)
cd "$TESTS_DIR" || exit 1

echo "Starting libsmbrr unit test suite..."
echo "Using test image: $IMAGE"
echo ""

# Invoke individual unit tests with their expected arguments
run_test "test_atrous" "$IMAGE" "test_out_atrous"
run_test "test_structures" "$IMAGE" "test_out_structures"
run_test "test_objects" "-i" "$IMAGE" "-o" "test_out_objects"
run_test "test_reconstruct" "-i" "$IMAGE" "-o" "test_out_reconstruct.bmp"

echo "============================================================"
echo "                        TEST SUMMARY                        "
echo "============================================================"
echo "Total Tests : $TOTAL"
if [ $FAILED -gt 0 ]; then
    echo -e "Passed      : \033[0;32m$PASSED\033[0m"
    echo -e "Failed      : \033[0;31m$FAILED\033[0m"
else
    echo -e "Passed      : \033[0;32m$PASSED\033[0m"
    echo "Failed      : $FAILED"
fi
echo "============================================================"

if [ $FAILED -gt 0 ]; then
    exit 1
else
    exit 0
fi
