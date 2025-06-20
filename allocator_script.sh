#!/bin/bash
set -e  # Exit on any error

echo "== Testing of memory allocator implemented in C programming language =="

# Clean previous builds
make clean

echo "1. Building with different configurations..."
make all  # This builds basic_test, valgrind_test, and asan_test

echo "2. Running basic functionality tests..."
make test
echo "✓ Basic tests passed"

echo "3. Running with sanitizers..."
make test-asan
echo "✓ Sanitizer tests passed"

echo "4. Running Valgrind analysis..."
make test-valgrind > valgrind.log 2>&1
if [ $? -eq 0 ]; then
    echo "✓ Valgrind tests passed"
else
    echo "✗ Valgrind found issues - check valgrind.log"
    exit 1
fi

echo "5. Running static analysis..."
if command -v cppcheck >/dev/null 2>&1; then
    cppcheck --error-exitcode=1 --enable=all --suppress=missingIncludeSystem allocator.c > cppcheck.log 2>&1
    if [ $? -eq 0 ]; then
        echo "✓ Static analysis passed"
    else
        echo "✗ Static analysis found issues - check cppcheck.log"
    fi
else
    echo "⚠ cppcheck not installed, skipping static analysis"
    echo "  Install with: sudo apt-get install cppcheck"
fi

echo "6. Memory usage analysis..."
make analyze-memory > memory_analysis.log 2>&1
if [ $? -eq 0 ]; then
    echo "✓ Memory analysis completed - check memory_analysis.log"
else
    echo "⚠ Memory analysis failed or tools not available"
fi

echo "7. Stress testing..."
make stress
echo "✓ Stress tests passed"

echo "8. Performance test..."
echo "Timing basic test execution:"
time make test > /dev/null
echo "✓ Performance test completed"

echo ""
echo "=== All tests completed successfully! ==="
echo ""
echo "Generated log files:"
echo "  - valgrind.log (Valgrind output)"
echo "  - cppcheck.log (Static analysis)"
echo "  - memory_analysis.log (Memory usage patterns)"
echo ""
echo "To view detailed results:"
echo "  cat valgrind.log"
echo "  cat cppcheck.log"
echo "  cat memory_analysis.log"