/* test_helpers.h - Helper functions for tests
 *
 * Copyright (C) 2024  Preload-NG Contributors
 *
 * This file is part of preload.
 */

#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* 
 * Get the path to bash or a shell that exists on this system.
 * Returns a static buffer - do not free.
 * Falls back through common paths, then uses SHELL env var.
 */
static const char *get_system_shell_path(void)
{
    static char shell_path[256] = {0};
    
    /* If already found, return cached result */
    if (shell_path[0] != '\0') {
        return shell_path;
    }
    
    /* List of common shell paths to try */
    static const char *candidates[] = {
        "/bin/bash",
        "/usr/bin/bash",
        "/bin/sh",
        "/usr/bin/sh",
        "/run/current-system/sw/bin/bash",  /* NixOS */
        NULL
    };
    
    /* Try each candidate */
    for (int i = 0; candidates[i] != NULL; i++) {
        if (access(candidates[i], X_OK) == 0) {
            strncpy(shell_path, candidates[i], sizeof(shell_path) - 1);
            return shell_path;
        }
    }
    
    /* Fallback to SHELL environment variable */
    const char *env_shell = getenv("SHELL");
    if (env_shell && access(env_shell, X_OK) == 0) {
        strncpy(shell_path, env_shell, sizeof(shell_path) - 1);
        return shell_path;
    }
    
    /* Last resort: return /bin/sh even if it might not exist */
    strncpy(shell_path, "/bin/sh", sizeof(shell_path) - 1);
    return shell_path;
}

/*
 * Get path to a library file that exists on this system.
 * Returns a static buffer - do not free.
 */
static const char *get_system_lib_path(void)
{
    static char lib_path[256] = {0};
    
    if (lib_path[0] != '\0') {
        return lib_path;
    }
    
    /* List of common library paths to try */
    static const char *candidates[] = {
        "/lib/x86_64-linux-gnu/libc.so.6",
        "/lib64/libc.so.6",
        "/lib/libc.so.6",
        "/usr/lib/libc.so.6",
        "/nix/store",  /* NixOS uses this, but we need a specific file */
        NULL
    };
    
    for (int i = 0; candidates[i] != NULL; i++) {
        if (access(candidates[i], R_OK) == 0) {
            strncpy(lib_path, candidates[i], sizeof(lib_path) - 1);
            return lib_path;
        }
    }
    
    /* Fallback to the shell path (which we know exists) */
    return get_system_shell_path();
}

#endif /* TEST_HELPERS_H */
