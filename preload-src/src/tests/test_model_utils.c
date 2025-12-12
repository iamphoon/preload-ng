/* test_model_utils.c - Unit tests for model validation utilities
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

#include "state.h"
#include "model_utils.h"
#include "exe.h"
#include "map.h"
#include "test_helpers.h"

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


/* Initialize minimal state for tests */
static void test_init_state(void)
{
    memset(state, 0, sizeof(*state));
    state->time = 100;
    state->last_running_timestamp = 90;
    state->exes = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, (GDestroyNotify)preload_exe_free);
    state->bad_exes = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    state->maps = g_hash_table_new((GHashFunc)preload_map_hash, (GEqualFunc)preload_map_equal);
    state->maps_arr = g_ptr_array_new();
}

static void test_cleanup_state(void)
{
    if (state->exes) g_hash_table_destroy(state->exes);
    if (state->bad_exes) g_hash_table_destroy(state->bad_exes);
    if (state->maps) g_hash_table_destroy(state->maps);
    if (state->maps_arr) g_ptr_array_free(state->maps_arr, TRUE);
    memset(state, 0, sizeof(*state));
}


static int test_validate_exe_exists(void)
{
    /* Use dynamically detected shell path */
    const char *shell = get_system_shell_path();
    int result = preload_validate_exe(shell, 0, 0);
    ASSERT_EQ(result, 0);
    
    return TEST_PASS;
}


static int test_validate_exe_missing(void)
{
    int result = preload_validate_exe("/nonexistent/path/to/exe", 0, 0);
    ASSERT_EQ(result, -1);
    
    return TEST_PASS;
}


static int test_validate_exe_invalid_path(void)
{
    /* Relative path is invalid */
    int result = preload_validate_exe("relative/path", 0, 0);
    ASSERT_EQ(result, -1);
    
    return TEST_PASS;
}


static int test_validate_exe_null(void)
{
    int result = preload_validate_exe(NULL, 0, 0);
    ASSERT_EQ(result, -1);
    
    return TEST_PASS;
}


static int test_validate_exe_empty(void)
{
    int result = preload_validate_exe("", 0, 0);
    ASSERT_EQ(result, -1);
    
    return TEST_PASS;
}


static int test_validate_map_exists(void)
{
    /* Use dynamically detected shell path */
    const char *shell = get_system_shell_path();
    int result = preload_validate_map(shell);
    ASSERT_EQ(result, 1);
    
    return TEST_PASS;
}


static int test_validate_map_missing(void)
{
    int result = preload_validate_map("/nonexistent/path/to/lib.so");
    ASSERT_EQ(result, 0);
    
    return TEST_PASS;
}


static int test_validate_map_pseudo_fs(void)
{
    /* /proc paths should always return valid */
    int result = preload_validate_map("/proc/self/maps");
    ASSERT_EQ(result, 1);
    
    result = preload_validate_map("/sys/devices");
    ASSERT_EQ(result, 1);
    
    result = preload_validate_map("/dev/null");
    ASSERT_EQ(result, 1);
    
    return TEST_PASS;
}


static int test_validate_map_null(void)
{
    int result = preload_validate_map(NULL);
    ASSERT_EQ(result, 0);
    
    return TEST_PASS;
}


static int test_cleanup_invalid_entries_null(void)
{
    /* NULL args should not crash and return 0 */
    int result = preload_cleanup_invalid_entries(NULL, NULL);
    ASSERT_EQ(result, 0);
    
    return TEST_PASS;
}


static int test_cleanup_invalid_entries_empty(void)
{
    test_init_state();
    
    /* Empty tables should be fine */
    int result = preload_cleanup_invalid_entries(state->exes, state->maps);
    ASSERT_EQ(result, 0);
    
    test_cleanup_state();
    
    return TEST_PASS;
}


int test_model_utils_run(void)
{
    int failed = 0;
    
    fprintf(stderr, "  Running test_validate_exe_exists... ");
    if (test_validate_exe_exists() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_validate_exe_missing... ");
    if (test_validate_exe_missing() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_validate_exe_invalid_path... ");
    if (test_validate_exe_invalid_path() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_validate_exe_null... ");
    if (test_validate_exe_null() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_validate_exe_empty... ");
    if (test_validate_exe_empty() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_validate_map_exists... ");
    if (test_validate_map_exists() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_validate_map_missing... ");
    if (test_validate_map_missing() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_validate_map_pseudo_fs... ");
    if (test_validate_map_pseudo_fs() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_validate_map_null... ");
    if (test_validate_map_null() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_cleanup_invalid_entries_null... ");
    if (test_cleanup_invalid_entries_null() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_cleanup_invalid_entries_empty... ");
    if (test_cleanup_invalid_entries_empty() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    return failed;
}
