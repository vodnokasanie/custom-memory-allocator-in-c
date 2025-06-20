# custom-memory-allocator-in-c
Implementation of my own custom allocator using C language

Custom Memory Allocator
A custom implementation of dynamic memory allocation functions (`malloc` and `free`) in C, designed for educational purposes and performance experimentation.
Overview
This allocator implements a heap-based memory management system using the `sbrk()` system call to manage memory directly from the operating system. The core of the allocator is based on the doubly-linked free list data structure using first fit technique. It provides an alternative to the standard library's `malloc()` and `free()` functions with additional debugging capabilities and explicit control over allocation strategies.
Features

- Custom heap management using `sbrk()` system calls
- First-fit allocation strategy for finding suitable free blocks
- Block splitting to minimize internal fragmentation
- Block coalescing to reduce external fragmentation
- Doubly-linked free list for efficient free block management
- Memory alignment (8-byte boundary) for optimal performance
- Corruption detection using magic numbers
- Debugging utilities for heap inspection and validation



Compiling, Debugging and Testing
To compile and debug the source code sanitizers and Valgrind were used. For testing automated shell script was executed to streamline building, testing, analyzing and profiling my custom memory allocator.

The Makefile defines multiple build targets and test workflows for your allocator, including support for:

- Basic build and test
- `Valgrind` memory checking
- `AddressSanitizer` and `UndefinedBehaviorSanitizer`
- Thread safety testing (optional)
- Performance profiling and memory usage analysis


The script automates the full workflow. It ensures the allocator is tested with various tools and configurations.

- Cleans previous builds with `make clean`
- Compiles all versions with `make all`
- Runs basic functionality tests using `make test`
- Executes tests under AddressSanitizer via `make test-asan`
- Runs Valgrind and logs output to `valgrind.log`
- Performs static analysis with cppcheck (if available), logs to `cppcheck.log`
- Analyzes memory usage using `valgrind --tool=massif`, logs to `memory_analysis.log`
- Stress tests the allocator with 10 consecutive executions
- Times performance of a test run using time

`valgrind.log`, `memory_analysis.log` and `massif.out` were generated as a result of running makefile wrapped up in the script.

The memory allocator works perfectly as zero memory leaks were detected as all tests passed. Also allocator showed a significant increase performance-wise comparing to system allocator. The difference is calculated up to 0.48 ratio which means that custom allocator is approximately 2.1x faster than the standard library's allocator for specific workload or test scenario.
