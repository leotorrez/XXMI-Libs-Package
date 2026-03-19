#include "ShaderCacheTest.h"
#include "log.h"

TEST(shader_type_encoding_roundtrip) {
    wchar_t type_in[5] = L"ps_5_0";
    uint32_t encoded = EncodeShaderType(type_in);
    ASSERT_NE(encoded, 0u);
    
    wchar_t type_out[5];
    DecodeShaderType(encoded, type_out, 5);
    ASSERT_STREQ(type_in, type_out);
}

TEST(shader_type_encoding_vs) {
    wchar_t type_in[5] = L"vs_5_0";
    uint32_t encoded = EncodeShaderType(type_in);
    ASSERT_NE(encoded, 0u);
    
    wchar_t type_out[5];
    DecodeShaderType(encoded, type_out, 5);
    ASSERT_STREQ(type_in, type_out);
}

TEST(shader_type_encoding_case_insensitive) {
    wchar_t type_lower[5] = L"ps_5_0";
    wchar_t type_upper[5] = L"PS_5_0";
    
    uint32_t encoded_lower = EncodeShaderType(type_lower);
    uint32_t encoded_upper = EncodeShaderType(type_upper);
    
    ASSERT_EQ(encoded_lower, encoded_upper);
}

TEST(shader_type_encoding_empty) {
    uint32_t encoded = EncodeShaderType(NULL);
    ASSERT_EQ(encoded, 0u);
    
    encoded = EncodeShaderType(L"");
    ASSERT_EQ(encoded, 0u);
}

TEST(init_and_close_cache) {
    cleanup_test_cache();
    wchar_t* path = get_test_cache_dir();
    
    SplitShaderCache* cache = InitSplitShaderCache(path, 0, 100, 10, 65536, 100, true, true);
    ASSERT_TRUE(cache != NULL);
    
    CloseSplitShaderCache(cache);
    cleanup_test_cache();
}

TEST(insert_and_query_shader) {
    cleanup_test_cache();
    wchar_t* path = get_test_cache_dir();
    
    SplitShaderCache* cache = InitSplitShaderCache(path, 0, 100, 10, 65536, 100, true, true);
    ASSERT_TRUE(cache != NULL);
    
    uint64_t hash = 0x1234567890ABCDEF;
    const uint8_t bytecode[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    FILETIME timestamp = {0};
    
    bool inserted = InsertSplitShaderToCache(cache, hash, L"ps_5_0", bytecode, sizeof(bytecode), timestamp);
    ASSERT_TRUE(inserted);
    
    uint32_t out_size = 0;
    const void* result = QuerySplitShaderBytecode(cache, hash, L"ps_5_0", &out_size);
    ASSERT_TRUE(result != NULL);
    ASSERT_EQ(out_size, sizeof(bytecode));
    ASSERT_TRUE(memcmp(result, bytecode, sizeof(bytecode)) == 0);
    
    FreeSplitShaderBytecode(cache, result);
    CloseSplitShaderCache(cache);
    cleanup_test_cache();
}

TEST(query_nonexistent_shader) {
    cleanup_test_cache();
    wchar_t* path = get_test_cache_dir();
    
    SplitShaderCache* cache = InitSplitShaderCache(path, 0, 100, 10, 65536, 100, true, true);
    ASSERT_TRUE(cache != NULL);
    
    uint32_t out_size = 0;
    const void* result = QuerySplitShaderBytecode(cache, 0xDEADBEEF, L"ps_5_0", &out_size);
    ASSERT_TRUE(result == NULL);
    
    CloseSplitShaderCache(cache);
    cleanup_test_cache();
}

TEST(duplicate_insert_skipped) {
    cleanup_test_cache();
    wchar_t* path = get_test_cache_dir();
    
    SplitShaderCache* cache = InitSplitShaderCache(path, 0, 100, 10, 65536, 100, true, true);
    ASSERT_TRUE(cache != NULL);
    
    uint64_t hash = 0x1234567890ABCDEF;
    const uint8_t bytecode1[] = {0x01, 0x02, 0x03};
    const uint8_t bytecode2[] = {0xFF, 0xFE, 0xFD};
    FILETIME timestamp = {0};
    
    bool inserted1 = InsertSplitShaderToCache(cache, hash, L"ps_5_0", bytecode1, sizeof(bytecode1), timestamp);
    ASSERT_TRUE(inserted1);
    
    bool inserted2 = InsertSplitShaderToCache(cache, hash, L"ps_5_0", bytecode2, sizeof(bytecode2), timestamp);
    ASSERT_TRUE(inserted2);
    
    uint32_t out_size = 0;
    const void* result = QuerySplitShaderBytecode(cache, hash, L"ps_5_0", &out_size);
    ASSERT_TRUE(result != NULL);
    ASSERT_EQ(out_size, sizeof(bytecode1));
    ASSERT_TRUE(memcmp(result, bytecode1, sizeof(bytecode1)) == 0);
    
    FreeSplitShaderBytecode(cache, result);
    CloseSplitShaderCache(cache);
    cleanup_test_cache();
}

TEST(multiple_shaders_different_types) {
    cleanup_test_cache();
    wchar_t* path = get_test_cache_dir();
    
    SplitShaderCache* cache = InitSplitShaderCache(path, 0, 100, 10, 65536, 100, true, true);
    ASSERT_TRUE(cache != NULL);
    
    const uint8_t ps_bytecode[] = {0x01, 0x02, 0x03};
    const uint8_t vs_bytecode[] = {0x04, 0x05, 0x06};
    FILETIME timestamp = {0};
    
    InsertSplitShaderToCache(cache, 0x1111, L"ps_5_0", ps_bytecode, sizeof(ps_bytecode), timestamp);
    InsertSplitShaderToCache(cache, 0x2222, L"vs_5_0", vs_bytecode, sizeof(vs_bytecode), timestamp);
    
    uint32_t ps_size = 0, vs_size = 0;
    const void* ps_result = QuerySplitShaderBytecode(cache, 0x1111, L"ps_5_0", &ps_size);
    const void* vs_result = QuerySplitShaderBytecode(cache, 0x2222, L"vs_5_0", &vs_size);
    
    ASSERT_TRUE(ps_result != NULL);
    ASSERT_TRUE(vs_result != NULL);
    ASSERT_TRUE(memcmp(ps_result, ps_bytecode, sizeof(ps_bytecode)) == 0);
    ASSERT_TRUE(memcmp(vs_result, vs_bytecode, sizeof(vs_bytecode)) == 0);
    
    FreeSplitShaderBytecode(cache, ps_result);
    FreeSplitShaderBytecode(cache, vs_result);
    CloseSplitShaderCache(cache);
    cleanup_test_cache();
}

TEST(regex_shader_storage) {
    cleanup_test_cache();
    wchar_t* path = get_test_cache_dir();
    
    SplitShaderCache* cache = InitSplitShaderCache(path, 0x12345678, 100, 10, 65536, 100, true, true);
    ASSERT_TRUE(cache != NULL);
    
    uint64_t hash = 0x1234567890ABCDEF;
    const uint8_t bytecode[] = {0x01, 0x02, 0x03, 0x04};
    uint32_t match_ids[] = {1, 2, 3};
    
    bool stored = StoreSplitShaderRegexBytecode(cache, hash, L"ps_5_0", bytecode, sizeof(bytecode), 3, match_ids);
    ASSERT_TRUE(stored);
    
    uint32_t out_size = 0, out_num_matches = 0;
    uint32_t* out_match_ids = NULL;
    const void* result = QuerySplitShaderRegexBytecode(cache, hash, L"ps_5_0", 0x12345678, &out_size, &out_num_matches, &out_match_ids);
    
    ASSERT_TRUE(result != NULL);
    ASSERT_EQ(out_size, sizeof(bytecode));
    ASSERT_EQ(out_num_matches, 3u);
    ASSERT_TRUE(out_match_ids != NULL);
    ASSERT_TRUE(memcmp(result, bytecode, sizeof(bytecode)) == 0);
    
    delete[] out_match_ids;
    FreeSplitShaderBytecode(cache, result);
    CloseSplitShaderCache(cache);
    cleanup_test_cache();
}

TEST(regex_hash_mismatch) {
    cleanup_test_cache();
    wchar_t* path = get_test_cache_dir();
    
    SplitShaderCache* cache = InitSplitShaderCache(path, 0x12345678, 100, 10, 65536, 100, true, true);
    ASSERT_TRUE(cache != NULL);
    
    uint64_t hash = 0x1234567890ABCDEF;
    const uint8_t bytecode[] = {0x01, 0x02, 0x03};
    uint32_t match_ids[] = {1};
    
    StoreSplitShaderRegexBytecode(cache, hash, L"ps_5_0", bytecode, sizeof(bytecode), 1, match_ids);
    
    uint32_t out_size = 0;
    const void* result = QuerySplitShaderRegexBytecode(cache, hash, L"ps_5_0", 0xFFFFFFFF, &out_size, NULL, NULL);
    ASSERT_TRUE(result == NULL);
    
    CloseSplitShaderCache(cache);
    cleanup_test_cache();
}

TEST(mark_shader_regex_processed) {
    cleanup_test_cache();
    wchar_t* path = get_test_cache_dir();
    
    SplitShaderCache* cache = InitSplitShaderCache(path, 0, 100, 10, 65536, 100, true, true);
    ASSERT_TRUE(cache != NULL);
    
    uint64_t hash = 0x1234567890ABCDEF;
    const uint8_t bytecode[] = {0x01, 0x02, 0x03};
    FILETIME timestamp = {0};
    
    InsertSplitShaderToCache(cache, hash, L"ps_5_0", bytecode, sizeof(bytecode), timestamp);
    
    bool marked = MarkSplitShaderRegexProcessed(cache, hash, L"ps_5_0");
    ASSERT_TRUE(marked);
    
    CloseSplitShaderCache(cache);
    cleanup_test_cache();
}

TEST(cache_statistics) {
    cleanup_test_cache();
    wchar_t* path = get_test_cache_dir();
    
    SplitShaderCache* cache = InitSplitShaderCache(path, 0, 100, 10, 65536, 100, true, true);
    ASSERT_TRUE(cache != NULL);
    
    const uint8_t bytecode[] = {0x01, 0x02, 0x03};
    FILETIME timestamp = {0};
    
    for (int i = 0; i < 10; i++) {
        InsertSplitShaderToCache(cache, 0x1000 + i, L"ps_5_0", bytecode, sizeof(bytecode), timestamp);
    }
    
    uint32_t shader_count = 0, block_count = 0;
    GetSplitCacheStatistics(cache, &shader_count, &block_count);
    ASSERT_EQ(shader_count, 10u);
    
    CloseSplitShaderCache(cache);
    cleanup_test_cache();
}

TEST(integrity_validation) {
    cleanup_test_cache();
    wchar_t* path = get_test_cache_dir();
    
    SplitShaderCache* cache = InitSplitShaderCache(path, 0, 100, 10, 65536, 100, true, true);
    ASSERT_TRUE(cache != NULL);
    
    const uint8_t bytecode[] = {0x01, 0x02, 0x03};
    FILETIME timestamp = {0};
    
    InsertSplitShaderToCache(cache, 0x1234, L"ps_5_0", bytecode, sizeof(bytecode), timestamp);
    
    uint32_t errors = ValidateSplitCacheIntegrity(cache);
    ASSERT_EQ(errors, 0u);
    
    CloseSplitShaderCache(cache);
    cleanup_test_cache();
}

void run_all_shader_cache_tests() {
    printf("\n========== Shader Cache Unit Tests ==========\n\n");
    
    RUN_TEST(shader_type_encoding_roundtrip);
    RUN_TEST(shader_type_encoding_vs);
    RUN_TEST(shader_type_encoding_case_insensitive);
    RUN_TEST(shader_type_encoding_empty);
    RUN_TEST(init_and_close_cache);
    RUN_TEST(insert_and_query_shader);
    RUN_TEST(query_nonexistent_shader);
    RUN_TEST(duplicate_insert_skipped);
    RUN_TEST(multiple_shaders_different_types);
    RUN_TEST(regex_shader_storage);
    RUN_TEST(regex_hash_mismatch);
    RUN_TEST(mark_shader_regex_processed);
    RUN_TEST(cache_statistics);
    RUN_TEST(integrity_validation);
    
    print_test_summary();
}
