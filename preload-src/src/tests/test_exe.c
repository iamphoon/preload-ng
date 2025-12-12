/* test_exe.c - Unit tests for executable management
 *
 * Copyright (C) 2024  Preload-NG Contributors
 *
 * This file is part of preload.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "state.h"
#include "exe.h"
#include "map.h"

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

#define ASSERT_NULL(ptr) do { \
    if ((ptr) != NULL) { \
        fprintf(stderr, "  FAIL: %s:%d: %s is not NULL\n", __FILE__, __LINE__, #ptr); \
        return TEST_FAIL; \
    } \
} while(0)

#define ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        fprintf(stderr, "  FAIL: %s:%d: %s is NULL\n", __FILE__, __LINE__, #ptr); \
        return TEST_FAIL; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        fprintf(stderr, "  FAIL: %s:%d: %s != %s (%d != %d)\n", __FILE__, __LINE__, #a, #b, (int)(a), (int)(b)); \
        return TEST_FAIL; \
    } \
} while(0)

#define ASSERT_STR_EQ(a, b) do { \
    if (strcmp((a), (b)) != 0) { \
        fprintf(stderr, "  FAIL: %s:%d: \"%s\" != \"%s\"\n", __FILE__, __LINE__, (a), (b)); \
        return TEST_FAIL; \
    } \
} while(0)


/* Initialize minimal state for tests */
static void test_init_state(void)
{
    memset(state, 0, sizeof(*state));
    state->time = 100;
    state->last_running_timestamp = 90;
    state->exes = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
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


static int test_exe_new_basic(void)
{
    test_init_state();
    
    preload_exe_t *exe = preload_exe_new("/usr/bin/test", FALSE, NULL);
    
    ASSERT_NOT_NULL(exe);
    ASSERT_NOT_NULL(exe->path);
    ASSERT_STR_EQ(exe->path, "/usr/bin/test");
    ASSERT_EQ(exe->time, 0);
    ASSERT_EQ(exe->running_timestamp, -1);
    ASSERT_NOT_NULL(exe->exemaps);
    ASSERT_NOT_NULL(exe->markovs);
    
    preload_exe_free(exe);
    test_cleanup_state();
    
    return TEST_PASS;
}


static int test_exe_new_with_maps(void)
{
    test_init_state();
    
    /* Create a map and exemaps array */
    preload_map_t *map = preload_map_new("/usr/lib/libc.so.6", 0, 4096);
    preload_map_ref(map);
    
    GPtrArray *exemaps = g_ptr_array_new();
    preload_exemap_t *exemap = preload_exemap_new(map);
    g_ptr_array_add(exemaps, exemap);
    
    preload_exe_t *exe = preload_exe_new("/usr/bin/test", FALSE, exemaps);
    
    ASSERT_NOT_NULL(exe);
    ASSERT_EQ(exe->exemaps->len, 1);
    ASSERT_EQ(exe->size, 4096);  /* Size calculated from maps */
    
    preload_exe_free(exe);
    test_cleanup_state();
    
    return TEST_PASS;
}


static int test_exe_new_running(void)
{
    test_init_state();
    
    preload_exe_t *exe = preload_exe_new("/usr/bin/test", TRUE, NULL);
    
    ASSERT_NOT_NULL(exe);
    ASSERT_EQ(exe->running_timestamp, state->last_running_timestamp);
    ASSERT_EQ(exe->update_time, state->last_running_timestamp);
    
    preload_exe_free(exe);
    test_cleanup_state();
    
    return TEST_PASS;
}


static int test_exe_is_running(void)
{
    test_init_state();
    
    preload_exe_t *exe = preload_exe_new("/usr/bin/test", FALSE, NULL);
    
    /* Not running initially */
    ASSERT_FALSE(exe_is_running(exe));
    
    /* Set as running */
    exe->running_timestamp = state->last_running_timestamp;
    ASSERT_TRUE(exe_is_running(exe));
    
    /* Set as not running */
    exe->running_timestamp = state->last_running_timestamp - 1;
    ASSERT_FALSE(exe_is_running(exe));
    
    preload_exe_free(exe);
    test_cleanup_state();
    
    return TEST_PASS;
}


static int test_exemap_new(void)
{
    test_init_state();
    
    preload_map_t *map = preload_map_new("/usr/lib/test.so", 0, 2048);
    preload_map_ref(map);
    
    preload_exemap_t *exemap = preload_exemap_new(map);
    
    ASSERT_NOT_NULL(exemap);
    ASSERT_EQ(exemap->map, map);
    ASSERT_TRUE(exemap->prob == 1.0);
    
    preload_exemap_free(exemap, NULL);
    test_cleanup_state();
    
    return TEST_PASS;
}


static int test_exemap_new_null(void)
{
    /* NULL map should return NULL */
    preload_exemap_t *exemap = preload_exemap_new(NULL);
    ASSERT_NULL(exemap);
    
    return TEST_PASS;
}


static int test_exemap_new_from_exe(void)
{
    test_init_state();
    
    preload_exe_t *exe = preload_exe_new("/usr/bin/test", FALSE, NULL);
    preload_map_t *map = preload_map_new("/usr/lib/test.so", 0, 1024);
    preload_map_ref(map);
    
    ASSERT_EQ(exe->exemaps->len, 0);
    ASSERT_EQ(exe->size, 0);
    
    preload_exemap_t *exemap = preload_exemap_new_from_exe(exe, map);
    
    ASSERT_NOT_NULL(exemap);
    ASSERT_EQ(exe->exemaps->len, 1);
    ASSERT_EQ(exe->size, 1024);
    
    preload_exe_free(exe);
    test_cleanup_state();
    
    return TEST_PASS;
}


static int test_exe_register_unregister(void)
{
    test_init_state();
    
    preload_exe_t *exe = preload_exe_new("/usr/bin/test", FALSE, NULL);
    
    ASSERT_EQ(g_hash_table_size(state->exes), 0);
    
    preload_state_register_exe(exe, FALSE);
    
    ASSERT_EQ(g_hash_table_size(state->exes), 1);
    ASSERT_TRUE(exe->seq > 0);
    
    preload_exe_t *found = g_hash_table_lookup(state->exes, "/usr/bin/test");
    ASSERT_EQ(found, exe);
    
    preload_state_unregister_exe(exe);
    
    ASSERT_EQ(g_hash_table_size(state->exes), 0);
    
    test_cleanup_state();
    
    return TEST_PASS;
}


static int test_exe_new_null_path(void)
{
    /* NULL path should return NULL */
    preload_exe_t *exe = preload_exe_new(NULL, FALSE, NULL);
    ASSERT_NULL(exe);
    
    return TEST_PASS;
}


static int test_exe_foreach_exemap(void)
{
    test_init_state();
    
    preload_exe_t *exe = preload_exe_new("/usr/bin/test", FALSE, NULL);
    
    /* Add a few maps */
    preload_map_t *map1 = preload_map_new("/usr/lib/a.so", 0, 1024);
    preload_map_ref(map1);
    preload_exemap_new_from_exe(exe, map1);
    
    preload_map_t *map2 = preload_map_new("/usr/lib/b.so", 0, 2048);
    preload_map_ref(map2);
    preload_exemap_new_from_exe(exe, map2);
    
    ASSERT_EQ(exe->exemaps->len, 2);
    ASSERT_EQ(exe->size, 3072);  /* 1024 + 2048 */
    
    preload_exe_free(exe);
    test_cleanup_state();
    
    return TEST_PASS;
}


int test_exe_run(void)
{
    int failed = 0;
    
    fprintf(stderr, "  Running test_exe_new_basic... ");
    if (test_exe_new_basic() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_exe_new_with_maps... ");
    if (test_exe_new_with_maps() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_exe_new_running... ");
    if (test_exe_new_running() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_exe_is_running... ");
    if (test_exe_is_running() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_exemap_new... ");
    if (test_exemap_new() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_exemap_new_null... ");
    if (test_exemap_new_null() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_exemap_new_from_exe... ");
    if (test_exemap_new_from_exe() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_exe_register_unregister... ");
    if (test_exe_register_unregister() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_exe_new_null_path... ");
    if (test_exe_new_null_path() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_exe_foreach_exemap... ");
    if (test_exe_foreach_exemap() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    return failed;
}
