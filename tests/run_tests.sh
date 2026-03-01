#!/bin/bash

# run_tests.sh
# A high-level script to invoke all libsmbrr unit tests and report results.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
TESTS_DIR="$SCRIPT_DIR"
IMAGE="${TESTS_DIR}/wiz-ha-x.bmp"

USE_FITS=0
if [ "$1" == "--fits" ]; then
    USE_FITS=1
    IMAGE="${TESTS_DIR}/ngc7380.fit"
fi

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
    local prefix="$2"
    shift 2

    local cmd=("./$test_name" "$@")
    
    echo "============================================================"
    echo "Running Test: $test_name (FITS: $USE_FITS)"
    echo "Command: ${cmd[*]}"
    echo "------------------------------------------------------------"
    
    # Run the command and let stdout/stderr pass through
    "${cmd[@]}"
    local status=$?
    
    echo "------------------------------------------------------------"
    if [ $status -eq 0 ]; then
        # Check generated images against ref
        local image_mismatch=0
        if [ $USE_FITS -eq 0 ]; then
            for ref_img in "$TESTS_DIR/ref/${prefix}"*.bmp; do
                if [ -f "$ref_img" ]; then
                    local base_img=$(basename "$ref_img")
                    if ! cmp -s "$ref_img" "$TESTS_DIR/$base_img"; then
                        echo "Image mismatch: $base_img"
                        image_mismatch=1
                    fi
                fi
            done
        fi
        
        if [ $image_mismatch -eq 0 ]; then
            echo -e "\033[0;32mRESULT: [ PASS ] $test_name\033[0m"
            ((PASSED++))
        else
            echo -e "\033[0;31mRESULT: [ FAIL ] $test_name (Image Mismatch)\033[0m"
            ((FAILED++))
        fi
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
run_test "test_atrous" "test_at-" "$IMAGE" "test_at"
run_test "test_structures" "test_st-" "$IMAGE" "test_st"
run_test "test_objects" "test_ob-" "-i" "$IMAGE" "-o" "test_ob"
run_test "test_reconstruct" "test_re" "-i" "$IMAGE" "-o" "test_re.bmp"
run_test "test_packages.sh" "none"

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
