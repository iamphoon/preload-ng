/* test_time_utils.c - Unit tests for time utilities
 *
 * Copyright (C) 2024  Preload-NG Contributors
 *
 * This file is part of preload.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>

#include "time_utils.h"

/* Test macros */
#define TEST_PASS 0
#define TEST_FAIL 1

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "  FAIL: %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        return TEST_FAIL; \
    } \
} while(0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        fprintf(stderr, "  FAIL: %s:%d: %s != %s (%d != %d)\n", __FILE__, __LINE__, #a, #b, (int)(a), (int)(b)); \
        return TEST_FAIL; \
    } \
} while(0)


static int test_check_boottime_support(void)
{
    int result = preload_check_boottime_support();
    
    /* Result should be 0 or 1 */
    ASSERT_TRUE(result == 0 || result == 1);
    
    return TEST_PASS;
}


static int test_boottime_cached(void)
{
    /* Calling twice should return the same value (cached) */
    int result1 = preload_check_boottime_support();
    int result2 = preload_check_boottime_support();
    
    ASSERT_EQ(result1, result2);
    
    return TEST_PASS;
}


static int test_get_boottime(void)
{
    int64_t boottime = preload_get_boottime();
    
    /* Should be a positive value (seconds since boot) */
    ASSERT_TRUE(boottime > 0);
    
    return TEST_PASS;
}


static int test_get_boottime_ms(void)
{
    int64_t boottime_ms = preload_get_boottime_ms();
    int64_t boottime_s = preload_get_boottime();
    
    /* Should be positive */
    ASSERT_TRUE(boottime_ms > 0);
    
    /* MS should be >= seconds * 1000 (approximately) */
    ASSERT_TRUE(boottime_ms >= boottime_s * 1000);
    
    /* And not too much larger (within 1 second) */
    ASSERT_TRUE(boottime_ms < (boottime_s + 1) * 1000);
    
    return TEST_PASS;
}


static int test_boottime_increases(void)
{
    int64_t t1 = preload_get_boottime_ms();
    
    /* Sleep a tiny bit */
    usleep(10000);  /* 10ms */
    
    int64_t t2 = preload_get_boottime_ms();
    
    /* Time should increase */
    ASSERT_TRUE(t2 >= t1);
    
    return TEST_PASS;
}


static int test_boottime_consistency(void)
{
    /* Get boottime in seconds and ms, verify consistency */
    int64_t sec = preload_get_boottime();
    int64_t ms = preload_get_boottime_ms();
    
    /* ms/1000 should approximately equal sec */
    int64_t sec_from_ms = ms / 1000;
    
    /* Allow 1 second tolerance for the calls */
    ASSERT_TRUE(sec_from_ms >= sec - 1);
    ASSERT_TRUE(sec_from_ms <= sec + 1);
    
    return TEST_PASS;
}


int test_time_utils_run(void)
{
    int failed = 0;
    
    fprintf(stderr, "  Running test_check_boottime_support... ");
    if (test_check_boottime_support() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_boottime_cached... ");
    if (test_boottime_cached() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_get_boottime... ");
    if (test_get_boottime() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_get_boottime_ms... ");
    if (test_get_boottime_ms() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_boottime_increases... ");
    if (test_boottime_increases() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_boottime_consistency... ");
    if (test_boottime_consistency() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    return failed;
}
