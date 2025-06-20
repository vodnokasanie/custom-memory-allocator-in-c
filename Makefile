# Makefile for testing custom memory allocator

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O0
VALGRIND_FLAGS = --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose
ASAN_FLAGS = -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer

# Source files
ALLOCATOR_SRC = allocator.c
TEST_SRC = allocator_tests.c
COMBINED_SRC = combined_test.c

# Executables
BASIC_TEST = basic_test
VALGRIND_TEST = valgrind_test
ASAN_TEST = asan_test
THREAD_TEST = thread_test

# Default target
all: $(BASIC_TEST) $(VALGRIND_TEST) $(ASAN_TEST)

# Create combined source file
$(COMBINED_SRC): $(ALLOCATOR_SRC) $(TEST_SRC)
	@echo "// Combined source file for testing" > $(COMBINED_SRC)
	@echo "#define _GNU_SOURCE" >> $(COMBINED_SRC)
	@cat $(ALLOCATOR_SRC) >> $(COMBINED_SRC)
	@echo "" >> $(COMBINED_SRC)
	@cat $(TEST_SRC) >> $(COMBINED_SRC)

# Basic test (no special tools)
$(BASIC_TEST): $(COMBINED_SRC)
	$(CC) $(CFLAGS) -o $(BASIC_TEST) $(COMBINED_SRC)

# Valgrind-compatible build (no optimizations)
$(VALGRIND_TEST): $(COMBINED_SRC)
	$(CC) $(CFLAGS) -DVALGRIND_TEST -o $(VALGRIND_TEST) $(COMBINED_SRC)

# AddressSanitizer build
$(ASAN_TEST): $(COMBINED_SRC)
	$(CC) $(CFLAGS) $(ASAN_FLAGS) -o $(ASAN_TEST) $(COMBINED_SRC)

# Thread safety test (if you add threading later)
$(THREAD_TEST): $(COMBINED_SRC)
	$(CC) $(CFLAGS) -pthread -DTHREAD_TEST -o $(THREAD_TEST) $(COMBINED_SRC)

# Test targets
test: $(BASIC_TEST)
	@echo "=== Running Basic Tests ==="
	./$(BASIC_TEST)

test-valgrind: $(VALGRIND_TEST)
	@echo "=== Running Valgrind Tests ==="
	@if command -v valgrind >/dev/null 2>&1; then \
		valgrind $(VALGRIND_FLAGS) ./$(VALGRIND_TEST); \
	else \
		echo "Valgrind not installed. Install with: sudo apt-get install valgrind"; \
	fi

test-asan: $(ASAN_TEST)
	@echo "=== Running AddressSanitizer Tests ==="
	./$(ASAN_TEST)

test-gdb: $(BASIC_TEST)
	@echo "=== Running GDB Test ==="
	@echo "run" | gdb -batch -ex "set confirm off" -x /dev/stdin ./$(BASIC_TEST)

# Memory analysis
analyze-memory: $(VALGRIND_TEST)
	@echo "=== Memory Analysis with Valgrind ==="
	@if command -v valgrind >/dev/null 2>&1; then \
		valgrind --tool=massif --massif-out-file=massif.out ./$(VALGRIND_TEST) && \
		ms_print massif.out; \
	else \
		echo "Valgrind not installed"; \
	fi

# Performance profiling
profile: $(BASIC_TEST)
	@echo "=== Performance Profiling ==="
	@if command -v perf >/dev/null 2>&1; then \
		perf record -g ./$(BASIC_TEST) && \
		perf report; \
	else \
		echo "perf not available. Install with: sudo apt-get install linux-tools-generic"; \
	fi

# Stress test
stress: $(BASIC_TEST)
	@echo "=== Stress Testing ==="
	@for i in {1..10}; do \
		echo "Stress test iteration $$i"; \
		./$(BASIC_TEST) || exit 1; \
	done

# Clean up
clean:
	rm -f $(BASIC_TEST) $(VALGRIND_TEST) $(ASAN_TEST) $(THREAD_TEST)
	rm -f $(COMBINED_SRC)
	rm -f massif.out perf.data perf.data.old
	rm -f *.core core.*

# Help
help:
	@echo "Available targets:"
	@echo "  all          - Build all test executables"
	@echo "  test         - Run basic tests"
	@echo "  test-valgrind - Run tests with Valgrind"
	@echo "  test-asan    - Run tests with AddressSanitizer"
	@echo "  test-gdb     - Run tests with GDB"
	@echo "  analyze-memory - Memory usage analysis"
	@echo "  profile      - Performance profiling"
	@echo "  stress       - Stress testing"
	@echo "  clean        - Clean up build files"

.PHONY: all test test-valgrind test-asan test-gdb analyze-memory profile stress clean help