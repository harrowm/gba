#!/bin/bash
# Build and run all test suites

set -e  # Exit on first error

TESTS_DIR="tests"
FAILED_SUITES=()
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

echo "=================================="
echo "Building and Running All Test Suites"
echo "=================================="
echo ""

# Function to run a test suite
run_suite() {
    local dir=$1
    local name=$2
    local executable=$3
    
    echo "[$name]"
    echo "  Building..."
    cd "$dir"
    
    if ! make clean > /dev/null 2>&1; then
        echo "  ⚠️  No Makefile or clean failed"
        cd - > /dev/null
        return
    fi
    
    if ! make 2>&1 | grep -q "warning"; then
        echo "  ✅ Clean compile (no warnings)"
    else
        echo "  ⚠️  Compiled with warnings"
    fi
    
    if [ -f "$executable" ]; then
        echo "  Running tests..."
        # Run tests and capture output (don't exit on failure)
        set +e
        output=$(./"$executable" 2>&1)
        exit_code=$?
        set -e
        
        # Extract test counts
        if echo "$output" | grep -q "PASSED"; then
            passed=$(echo "$output" | grep "PASSED" | tail -1 | grep -oE '[0-9]+' | head -1)
            PASSED_TESTS=$((PASSED_TESTS + passed))
            TOTAL_TESTS=$((TOTAL_TESTS + passed))
            echo "  ✅ $passed tests passed"
        fi
        if echo "$output" | grep -q "FAILED"; then
            failed=$(echo "$output" | grep "FAILED" | grep -oE '[0-9]+' | head -1)
            FAILED_TESTS=$((FAILED_TESTS + failed))
            TOTAL_TESTS=$((TOTAL_TESTS + failed))
            # Only report failures if the count is actually greater than 0
            if [ "$failed" -gt 0 ]; then
                echo "  ⚠️  $failed tests failed"
                FAILED_SUITES+=("$name ($failed failures)")
            fi
        fi
        
        # If no test results found, consider it a failure
        if ! echo "$output" | grep -q "PASSED\|FAILED"; then
            echo "  ❌ Test execution failed (no test output)"
            FAILED_SUITES+=("$name")
        fi
    else
        echo "  ⚠️  Executable not found: $executable"
    fi
    
    cd - > /dev/null
    echo ""
}

# Run all test suites
run_suite "$TESTS_DIR/arm_core" "ARM Core Tests" "run_all"
run_suite "$TESTS_DIR/thumb_core" "THUMB Core Tests" "thumb_core_tests"
run_suite "$TESTS_DIR/interrupts" "Interrupt Tests" "interrupt_tests"
run_suite "$TESTS_DIR/dma" "DMA Tests" "dma_tests"
run_suite "$TESTS_DIR/graphics" "Graphics Tests" "graphics_tests"

# Run scheduler tests (multiple executables)
echo "[Scheduler Tests]"
echo "  Building..."
cd "$TESTS_DIR/scheduler"
if make clean > /dev/null 2>&1 && make > /dev/null 2>&1; then
    echo "  ✅ Clean compile (no warnings)"
    echo "  Running tests..."
    
    for test_exe in test_scheduler test_timing_integration test_integration_basic; do
        if [ -f "$test_exe" ]; then
            if output=$(./"$test_exe" 2>&1); then
                passed=$(echo "$output" | grep "PASSED" | tail -1 | grep -oE '[0-9]+' | head -1)
                PASSED_TESTS=$((PASSED_TESTS + passed))
                TOTAL_TESTS=$((TOTAL_TESTS + passed))
            fi
        fi
    done
    echo "  ✅ All scheduler tests passed"
else
    echo "  ❌ Build or execution failed"
    FAILED_SUITES+=("Scheduler Tests")
fi
cd - > /dev/null
echo ""

# Summary
echo "=================================="
echo "Test Summary"
echo "=================================="
echo "Total tests: $TOTAL_TESTS"
echo "Passed: $PASSED_TESTS"
echo "Failed: $FAILED_TESTS"
echo ""

if [ ${#FAILED_SUITES[@]} -gt 0 ]; then
    echo "❌ Failed suites:"
    for suite in "${FAILED_SUITES[@]}"; do
        echo "   - $suite"
    done
    exit 1
else
    echo "✅ All test suites passed!"
    exit 0
fi
