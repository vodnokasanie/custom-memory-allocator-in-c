#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include "allocator.h"

// Include your allocator implementation here
// #include "memory_allocator.c"

// Test framework macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s - %s\n", __func__, message); \
            return 0; \
        } \
    } while(0)

#define TEST_PASS() \
    do { \
        printf("PASS: %s\n", __func__); \
        return 1; \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        printf("Running %s... ", #test_func); \
        if (test_func()) { \
            tests_passed++; \
        } \
        tests_total++; \
    } while(0)

// Global test counters
int tests_passed = 0;
int tests_total = 0;

// Test 1: Basic allocation and deallocation
int test_basic_allocation() {
    void* ptr1 = my_malloc(100);
    TEST_ASSERT(ptr1 != NULL, "Failed to allocate 100 bytes");
    
    void* ptr2 = my_malloc(200);
    TEST_ASSERT(ptr2 != NULL, "Failed to allocate 200 bytes");
    
    TEST_ASSERT(ptr1 != ptr2, "Pointers should be different");
    
    my_free(ptr1);
    my_free(ptr2);
    
    TEST_PASS();
}

// Test 2: Zero and large allocations
int test_edge_cases() {
    void* ptr_zero = my_malloc(0);
    TEST_ASSERT(ptr_zero == NULL, "malloc(0) should return NULL");
    
    void* ptr_large = my_malloc(1024 * 1024); // 1MB
    TEST_ASSERT(ptr_large != NULL, "Failed to allocate large block");
    
    my_free(ptr_large);
    my_free(NULL); // Should not crash
    
    TEST_PASS();
}

// Test 3: Memory reuse after free
int test_memory_reuse() {
    void* ptr1 = my_malloc(100);
    void* original_ptr1 = ptr1;
    
    my_free(ptr1);
    
    void* ptr2 = my_malloc(50); // Smaller allocation
    TEST_ASSERT(ptr2 != NULL, "Failed to allocate after free");
    
    // Should reuse the same space (first-fit)
    TEST_ASSERT(ptr2 == original_ptr1, "Memory not reused properly");
    
    my_free(ptr2);
    TEST_PASS();
}

// Test 4: Coalescing adjacent blocks
int test_coalescing() {
    void* ptr1 = my_malloc(100);
    void* ptr2 = my_malloc(100);
    void* ptr3 = my_malloc(100);
    
    TEST_ASSERT(ptr1 && ptr2 && ptr3, "Failed to allocate test blocks");
    
    // Free middle block first
    my_free(ptr2);
    
    // Free adjacent blocks - should coalesce
    my_free(ptr1);
    my_free(ptr3);
    
    // Try to allocate a large block that would only fit if coalescing worked
    void* large_ptr = my_malloc(250);
    TEST_ASSERT(large_ptr != NULL, "Coalescing failed - large allocation unsuccessful");
    
    my_free(large_ptr);
    TEST_PASS();
}

// Test 5: Write and read data
int test_data_integrity() {
    char* ptr = (char*)my_malloc(1000);
    TEST_ASSERT(ptr != NULL, "Failed to allocate memory for data test");
    
    // Write pattern
    for (int i = 0; i < 1000; i++) {
        ptr[i] = (char)(i % 256);
    }
    
    // Verify pattern
    for (int i = 0; i < 1000; i++) {
        TEST_ASSERT(ptr[i] == (char)(i % 256), "Data corruption detected");
    }
    
    my_free(ptr);
    TEST_PASS();
}

// Test 6: Fragmentation handling
int test_fragmentation() {
    void* ptrs[10];
    
    // Allocate multiple small blocks
    for (int i = 0; i < 10; i++) {
        ptrs[i] = my_malloc(64);
        TEST_ASSERT(ptrs[i] != NULL, "Failed to allocate small block");
    }
    
    // Free every other block to create fragmentation
    for (int i = 1; i < 10; i += 2) {
        my_free(ptrs[i]);
        ptrs[i] = NULL;
    }
    
    // Try to allocate a block that fits in the fragmented space
    void* frag_ptr = my_malloc(32);
    TEST_ASSERT(frag_ptr != NULL, "Failed to handle fragmentation");
    
    // Clean up
    my_free(frag_ptr);
    for (int i = 0; i < 10; i += 2) {
        my_free(ptrs[i]);
    }
    
    TEST_PASS();
}

// Test 7: Stress test with random allocations
int test_stress() {
    srand(time(NULL));
    void* ptrs[100];
    int allocated_count = 0;
    
    for (int i = 0; i < 1000; i++) {
        int action = rand() % 3;
        
        if (action == 0 && allocated_count < 100) {
            // Allocate
            size_t size = (rand() % 1000) + 1;
            ptrs[allocated_count] = my_malloc(size);
            if (ptrs[allocated_count] != NULL) {
                // Write some data to test integrity
                memset(ptrs[allocated_count], allocated_count % 256, size);
                allocated_count++;
            }
        } else if (action == 1 && allocated_count > 0) {
            // Free random block
            int index = rand() % allocated_count;
            my_free(ptrs[index]);
            ptrs[index] = ptrs[allocated_count - 1];
            allocated_count--;
        }
        
        // Validate heap every 100 operations
        if (i % 100 == 0) {
            TEST_ASSERT(validate_heap(), "Heap corruption during stress test");
        }
    }
    
    // Clean up remaining allocations
    for (int i = 0; i < allocated_count; i++) {
        my_free(ptrs[i]);
    }
    
    TEST_PASS();
}

// Test 8: Alignment test
int test_alignment() {
    for (int i = 1; i <= 100; i++) {
        void* ptr = my_malloc(i);
        TEST_ASSERT(ptr != NULL, "Allocation failed");
        TEST_ASSERT(((uintptr_t)ptr % 8) == 0, "Pointer not properly aligned");
        my_free(ptr);
    }
    
    TEST_PASS();
}

// Test 9: Double free detection
int test_double_free() {
    void* ptr = my_malloc(100);
    TEST_ASSERT(ptr != NULL, "Failed to allocate");
    
    my_free(ptr);
    
    // This should be detected and handled gracefully
    // (Your implementation prints error but doesn't crash)
    my_free(ptr); // Double free
    
    TEST_PASS();
}

// Performance comparison test
void performance_test() {
    printf("\n=== Performance Test ===\n");
    
    struct timeval start, end;
    const int iterations = 10000;
    
    // Test custom allocator
    gettimeofday(&start, NULL);
    for (int i = 0; i < iterations; i++) {
        void* ptr = my_malloc(rand() % 1000 + 1);
        if (ptr) {
            my_free(ptr);
        }
    }
    gettimeofday(&end, NULL);
    
    long custom_time = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    
    // Test system allocator
    gettimeofday(&start, NULL);
    for (int i = 0; i < iterations; i++) {
        void* ptr = malloc(rand() % 1000 + 1);
        if (ptr) {
            free(ptr);
        }
    }
    gettimeofday(&end, NULL);
    
    long system_time = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    
    printf("Custom allocator: %ld microseconds\n", custom_time);
    printf("System allocator: %ld microseconds\n", system_time);
    printf("Ratio: %.2fx %s\n", 
           (double)custom_time / system_time,
           custom_time > system_time ? "slower" : "faster");
}

// Memory usage analysis
void memory_usage_test() {
    printf("\n=== Memory Usage Analysis ===\n");
    
    print_heap_debug();
    
    printf("Allocating various sizes...\n");
    void* ptrs[5];
    ptrs[0] = my_malloc(64);
    ptrs[1] = my_malloc(128);
    ptrs[2] = my_malloc(256);
    ptrs[3] = my_malloc(512);
    ptrs[4] = my_malloc(1024);
    
    print_heap_debug();
    
    printf("Freeing middle allocations...\n");
    my_free(ptrs[1]);
    my_free(ptrs[3]);
    
    print_heap_debug();
    
    printf("Allocating to test reuse...\n");
    void* reuse_ptr = my_malloc(100);
    
    print_heap_debug();
    
    // Cleanup
    my_free(ptrs[0]);
    my_free(ptrs[2]);
    my_free(ptrs[4]);
    my_free(reuse_ptr);
}

// Main test runner
int main() {
    printf("=== Custom Memory Allocator Test Suite ===\n\n");
    
    // Run all tests
    RUN_TEST(test_basic_allocation);
    RUN_TEST(test_edge_cases);
    RUN_TEST(test_memory_reuse);
    RUN_TEST(test_coalescing);
    RUN_TEST(test_data_integrity);
    RUN_TEST(test_fragmentation);
    RUN_TEST(test_alignment);
    RUN_TEST(test_double_free);
    RUN_TEST(test_stress);
    
    // Results
    printf("\n=== Test Results ===\n");
    printf("Passed: %d/%d tests\n", tests_passed, tests_total);
    
    if (tests_passed == tests_total) {
        printf("All tests passed! ✓\n");
    } else {
        printf("Some tests failed! ✗\n");
    }
    
    // Additional analysis
    performance_test();
    memory_usage_test();
    
    printf("\nFinal heap validation: %s\n", 
           validate_heap() ? "PASSED" : "FAILED");
    
    return (tests_passed == tests_total) ? 0 : 1;
}