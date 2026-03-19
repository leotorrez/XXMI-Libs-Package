#pragma once
#ifndef SHADER_CACHE_TEST_H
#define SHADER_CACHE_TEST_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "ShaderCacheSplit.h"

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

struct TestResult {
    const char* name;
    bool passed;
    const char* message;
};

#define MAX_TESTS 100
static TestResult g_test_results[MAX_TESTS];
static int g_test_count = 0;

static void add_test_result(const char* name, bool passed, const char* message = nullptr) {
    if (g_test_count < MAX_TESTS) {
        g_test_results[g_test_count].name = name;
        g_test_results[g_test_count].passed = passed;
        g_test_results[g_test_count].message = message;
        g_test_count++;
    }
}

#define TEST(name) void test_##name(); \
    struct TestReg_##name { TestReg_##name() { add_test_result(#name, false); } } g_test_reg_##name; \
    void test_##name()

#define RUN_TEST(name) do { \
    printf("Running %s... ", #name); \
    test_##name(); \
    for (int i = 0; i < g_test_count; i++) { \
        if (strcmp(g_test_results[i].name, #name) == 0 && g_test_results[i].message == nullptr) { \
            g_test_results[i].passed = true; \
            printf("PASSED\n"); \
            break; \
        } \
    } \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        printf("FAILED: %s (line %d)\n", #cond, __LINE__); \
        return; \
    } \
} while(0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))
#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))
#define ASSERT_NE(a, b) ASSERT_TRUE((a) != (b))
#define ASSERT_STREQ(a, b) ASSERT_TRUE(strcmp((a), (b)) == 0)

static wchar_t* get_test_cache_dir() {
    static wchar_t path[MAX_PATH];
    GetTempPathW(MAX_PATH, path);
    wcscat_s(path, MAX_PATH, L"ShaderCacheTest\\");
    return path;
}

static void cleanup_test_cache() {
    wchar_t* path = get_test_cache_dir();
    RemoveDirectoryW(path);
}

static void print_test_summary() {
    printf("\n========== Test Summary ==========\n");
    int passed = 0, failed = 0;
    for (int i = 0; i < g_test_count; i++) {
        if (g_test_results[i].passed) {
            passed++;
            printf("[PASS] %s\n", g_test_results[i].name);
        } else {
            failed++;
            printf("[FAIL] %s", g_test_results[i].name);
            if (g_test_results[i].message) {
                printf(": %s", g_test_results[i].message);
            }
            printf("\n");
        }
    }
    printf("\nTotal: %d passed, %d failed\n", passed, failed);
    printf("===============================\n");
}

#endif // SHADER_CACHE_TEST_H
