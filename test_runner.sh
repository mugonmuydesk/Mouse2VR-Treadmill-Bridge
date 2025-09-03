#!/bin/bash

# Script to run Windows tests from WSL and analyze results

echo "=========================================="
echo "Mouse2VR Test Runner (WSL → Windows)"
echo "=========================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Ensure we're in the right directory
cd /mnt/c/Dev/Mouse2VR-Treadmill-Bridge

# Run Windows batch file
echo "Starting Windows test execution..."
cmd.exe /c "run_tests.bat" 2>&1 | tee test_output.log
TEST_EXIT_CODE=${PIPESTATUS[0]}

echo ""
echo "=========================================="
echo "Test Analysis"
echo "=========================================="

# Check if XML results exist
if [ -f "build/test_results.xml" ]; then
    echo "Parsing test results..."
    
    # Extract test counts from XML
    TOTAL_TESTS=$(grep -o 'tests="[0-9]*"' build/test_results.xml | head -1 | grep -o '[0-9]*')
    FAILURES=$(grep -o 'failures="[0-9]*"' build/test_results.xml | head -1 | grep -o '[0-9]*')
    ERRORS=$(grep -o 'errors="[0-9]*"' build/test_results.xml | head -1 | grep -o '[0-9]*')
    TIME=$(grep -o 'time="[0-9.]*"' build/test_results.xml | head -1 | grep -o '[0-9.]*')
    
    echo "Total Tests: $TOTAL_TESTS"
    echo "Failures: $FAILURES"
    echo "Errors: $ERRORS"
    echo "Time: ${TIME}s"
    echo ""
    
    # Show failed tests if any
    if [ "$FAILURES" -gt 0 ] || [ "$ERRORS" -gt 0 ]; then
        echo -e "${RED}Failed Tests:${NC}"
        grep -A 2 '<failure' build/test_results.xml | grep 'message=' | sed 's/.*message="\(.*\)".*/  - \1/'
    fi
    
    # Overall result
    if [ $TEST_EXIT_CODE -eq 0 ]; then
        echo -e "${GREEN}✓ ALL TESTS PASSED${NC}"
    else
        echo -e "${RED}✗ TESTS FAILED${NC}"
    fi
else
    echo -e "${YELLOW}Warning: No XML results found${NC}"
    
    # Try to parse console output
    if [ -f "test_output.log" ]; then
        echo "Checking console output..."
        if grep -q "ALL TESTS PASSED" test_output.log; then
            echo -e "${GREEN}✓ Tests appear to have passed${NC}"
        elif grep -q "SOME TESTS FAILED" test_output.log; then
            echo -e "${RED}✗ Some tests failed${NC}"
            echo ""
            echo "Failed test output:"
            grep -A 5 "\[ FAILED \]" test_output.log
        fi
    fi
fi

# Create JSON summary for automation
cat > test_summary.json <<EOF
{
  "timestamp": "$(date -Iseconds)",
  "exit_code": $TEST_EXIT_CODE,
  "total_tests": ${TOTAL_TESTS:-0},
  "failures": ${FAILURES:-0},
  "errors": ${ERRORS:-0},
  "duration": ${TIME:-0}
}
EOF

echo ""
echo "Summary saved to: test_summary.json"
echo "Full log saved to: test_output.log"

exit $TEST_EXIT_CODE