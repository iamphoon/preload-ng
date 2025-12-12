/* test_map.c - Unit tests for memory map management
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


static int test_map_new_basic(void)
{
    test_init_state();
    
    preload_map_t *map = preload_map_new("/usr/lib/libc.so.6", 0, 4096);
    
    ASSERT_NOT_NULL(map);
    ASSERT_NOT_NULL(map->path);
    ASSERT_STR_EQ(map->path, "/usr/lib/libc.so.6");
    ASSERT_EQ(map->offset, 0);
    ASSERT_EQ(map->length, 4096);
    ASSERT_EQ(map->refcount, 0);
    ASSERT_EQ(map->update_time, state->time);
    
    preload_map_free(map);
    test_cleanup_state();
    
    return TEST_PASS;
}


static int test_map_new_null_path(void)
{
    preload_map_t *map = preload_map_new(NULL, 0, 1024);
    ASSERT_NULL(map);
    
    return TEST_PASS;
}


static int test_map_new_with_offset(void)
{
    test_init_state();
    
    preload_map_t *map = preload_map_new("/usr/lib/test.so", 8192, 2048);
    
    ASSERT_NOT_NULL(map);
    ASSERT_EQ(map->offset, 8192);
    ASSERT_EQ(map->length, 2048);
    
    preload_map_free(map);
    test_cleanup_state();
    
    return TEST_PASS;
}


static int test_map_ref_unref(void)
{
    test_init_state();
    
    preload_map_t *map = preload_map_new("/usr/lib/test.so", 0, 1024);
    ASSERT_EQ(map->refcount, 0);
    
    /* First ref registers the map */
    preload_map_ref(map);
    ASSERT_EQ(map->refcount, 1);
    ASSERT_TRUE(map->seq > 0);
    ASSERT_EQ(g_hash_table_size(state->maps), 1);
    ASSERT_EQ(state->maps_arr->len, 1);
    
    /* Unref to 0 unregisters and frees */
    preload_map_unref(map);
    ASSERT_EQ(g_hash_table_size(state->maps), 0);
    ASSERT_EQ(state->maps_arr->len, 0);
    
    test_cleanup_state();
    
    return TEST_PASS;
}


static int test_map_double_ref(void)
{
    test_init_state();
    
    preload_map_t *map = preload_map_new("/usr/lib/test.so", 0, 1024);
    
    preload_map_ref(map);
    ASSERT_EQ(map->refcount, 1);
    
    preload_map_ref(map);
    ASSERT_EQ(map->refcount, 2);
    
    /* Still only one entry in hash table */
    ASSERT_EQ(g_hash_table_size(state->maps), 1);
    
    preload_map_unref(map);
    ASSERT_EQ(map->refcount, 1);
    ASSERT_EQ(g_hash_table_size(state->maps), 1);  /* Still registered */
    
    preload_map_unref(map);  /* This frees */
    ASSERT_EQ(g_hash_table_size(state->maps), 0);
    
    test_cleanup_state();
    
    return TEST_PASS;
}


static int test_map_get_size(void)
{
    test_init_state();
    
    preload_map_t *map = preload_map_new("/usr/lib/test.so", 0, 8192);
    
    size_t size = preload_map_get_size(map);
    ASSERT_EQ(size, 8192);
    
    preload_map_free(map);
    test_cleanup_state();
    
    return TEST_PASS;
}


static int test_map_get_size_null(void)
{
    size_t size = preload_map_get_size(NULL);
    ASSERT_EQ(size, 0);
    
    return TEST_PASS;
}


static int test_map_hash(void)
{
    test_init_state();
    
    preload_map_t *map = preload_map_new("/usr/lib/test.so", 0, 1024);
    
    guint hash1 = preload_map_hash(map);
    guint hash2 = preload_map_hash(map);
    
    /* Hash should be consistent */
    ASSERT_EQ(hash1, hash2);
    ASSERT_TRUE(hash1 != 0);
    
    preload_map_free(map);
    test_cleanup_state();
    
    return TEST_PASS;
}


static int test_map_hash_null_safety(void)
{
    guint hash = preload_map_hash(NULL);
    ASSERT_EQ(hash, 0);
    
    return TEST_PASS;
}


static int test_map_equal_same(void)
{
    test_init_state();
    
    preload_map_t *map = preload_map_new("/usr/lib/test.so", 0, 1024);
    
    ASSERT_TRUE(preload_map_equal(map, map));
    
    preload_map_free(map);
    test_cleanup_state();
    
    return TEST_PASS;
}


static int test_map_equal_different_path(void)
{
    test_init_state();
    
    preload_map_t *map1 = preload_map_new("/usr/lib/a.so", 0, 1024);
    preload_map_t *map2 = preload_map_new("/usr/lib/b.so", 0, 1024);
    
    ASSERT_FALSE(preload_map_equal(map1, map2));
    
    preload_map_free(map1);
    preload_map_free(map2);
    test_cleanup_state();
    
    return TEST_PASS;
}


static int test_map_equal_different_offset(void)
{
    test_init_state();
    
    preload_map_t *map1 = preload_map_new("/usr/lib/test.so", 0, 1024);
    preload_map_t *map2 = preload_map_new("/usr/lib/test.so", 4096, 1024);
    
    ASSERT_FALSE(preload_map_equal(map1, map2));
    
    preload_map_free(map1);
    preload_map_free(map2);
    test_cleanup_state();
    
    return TEST_PASS;
}


static int test_map_equal_null_handling(void)
{
    test_init_state();
    
    preload_map_t *map = preload_map_new("/usr/lib/test.so", 0, 1024);
    
    ASSERT_FALSE(preload_map_equal(map, NULL));
    ASSERT_FALSE(preload_map_equal(NULL, map));
    ASSERT_TRUE(preload_map_equal(NULL, NULL));
    
    preload_map_free(map);
    test_cleanup_state();
    
    return TEST_PASS;
}


int test_map_run(void)
{
    int failed = 0;
    
    fprintf(stderr, "  Running test_map_new_basic... ");
    if (test_map_new_basic() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_map_new_null_path... ");
    if (test_map_new_null_path() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_map_new_with_offset... ");
    if (test_map_new_with_offset() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_map_ref_unref... ");
    if (test_map_ref_unref() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_map_double_ref... ");
    if (test_map_double_ref() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_map_get_size... ");
    if (test_map_get_size() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_map_get_size_null... ");
    if (test_map_get_size_null() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_map_hash... ");
    if (test_map_hash() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_map_hash_null_safety... ");
    if (test_map_hash_null_safety() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_map_equal_same... ");
    if (test_map_equal_same() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_map_equal_different_path... ");
    if (test_map_equal_different_path() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_map_equal_different_offset... ");
    if (test_map_equal_different_offset() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    fprintf(stderr, "  Running test_map_equal_null_handling... ");
    if (test_map_equal_null_handling() == TEST_PASS) {
        fprintf(stderr, "PASS\n");
    } else {
        failed++;
    }
    
    return failed;
}
