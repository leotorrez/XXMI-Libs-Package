# Shader Split Cache Implementation Review

## Overview

The split shader cache implementation (`ShaderCacheSplit.h` and `ShaderCacheSplit.cpp`) provides a two-tier caching system:
- **Index file** (`ShaderCache.idx`) - Lightweight header + lookup table
- **Block files** (`ShaderCache_NNNN.bin`) - Actual shader bytecode grouped in chunks

---

## Critical Issues

### 1. Race Condition in Block File Assignment (HIGH)

**Location:** `InsertSplitShaderToCache()` (line 619) and `StoreSplitShaderRegexBytecode()` (line 929)

**Problem:** The block file ID calculation and header update is not atomic:

```cpp
// Line 641-645: Calculates block file outside lock
uint32_t block_file_id = cache->header.shader_count / cache->header.shaders_per_block;
if (block_file_id >= cache->header.block_file_count) {
    cache->header.block_file_count = block_file_id + 1;
}
```

The lock is released before file I/O (line 666/1003) and re-acquired after. Between these points, another thread could:
1. Calculate the same block file ID
2. Write to the same block file
3. Cause data corruption or index mismatch

**Fix:** Reorganize to hold the lock during the entire operation, or use file-level locking.

---

### 2. LRU Cache Inconsistency (HIGH)

**Location:** `GetBlockFileHandle()` (line 76-125)

**Problem:** The LRU list update is duplicated and the logic is buggy:

```cpp
// Line 84-88: Removes and re-inserts at front
cache->block_file_lru.erase(
    std::remove(cache->block_file_lru.begin(), 
                cache->block_file_lru.end(), block_id),
    cache->block_file_lru.end());
cache->block_file_lru.insert(cache->block_file_lru.begin(), block_id);
```

Then at line 121, it inserts again without checking if it was already moved to front. This can cause duplicate entries.

**Fix:** Only insert if not already in the LRU list, or restructure to avoid double-handling.

---

### 3. Memory Pool Fragmentation (MEDIUM)

**Location:** `AllocPoolMemory()` (line 191-221)

**Problem:** The memory pool uses fixed-size blocks and a simple first-fit allocation strategy:
- Blocks are never coalesced
- When a large allocation returns a block, the remaining space is wasted
- No defragmentation mechanism

**Fix:** Consider using a slab allocator pattern or limit pool usage to shader-sized allocations only.

---

### 4. Missing Error Handling in Migration (MEDIUM)

**Location:** `MigrateMonolithicToSplit()` (line 1301-1534)

**Problem:** If migration fails partway through, the partial cache may be in an inconsistent state:
- Index entries may reference shaders not yet migrated
- Block files may have incomplete data

**Fix:** Use a transaction pattern - write to a temporary directory first, then atomically rename.

---

### 5. Memory Mapping Not Tracked by LRU (MEDIUM)

**Location:** `block_mappings` vs `open_block_files` (ShaderCacheSplit.h:104, 104)

**Problem:** `block_mappings` (memory-mapped views) are not tracked in the LRU list. When we close LRU block files (line 111-117), we don't unmap their memory views. This can lead to:
- Memory-mapped views staying open indefinitely
- Wasted address space
- Potential handle leaks on 32-bit systems

**Fix:** Track memory mappings in the same LRU structure, or unmap when closing handles.

---

## Code Quality Issues

### 1. Inconsistent Locking Patterns

**Location:** Multiple functions

Some functions release the lock during I/O:
- `InsertSplitShaderToCache()` (line 666)
- `StoreSplitShaderRegexBytecode()` (line 1003)

Others hold the lock during I/O:
- `QuerySplitShaderBytecode()` (holds lock for file I/O fallback)
- `QuerySplitShaderRegexBytecode()` (holds lock for file I/O fallback)

**Recommendation:** Choose one pattern consistently. If releasing lock during I/O:
1. Use atomic operations for shared state updates
2. Consider a per-block-file locking strategy instead of global lock

---

### 2. Magic Numbers Without Constants

**Location:** Multiple files

```cpp
// ShaderCacheSplit.cpp:514
if (block_header->magic == 0x53444342)

// Should be:
#define SHADER_BLOCK_MAGIC 0x53444342
```

**Recommendation:** Extract all magic numbers to named constants at the top of the file.

---

### 3. No Validation of `num_matches` Bound Check

**Location:** `QuerySplitShaderRegexBytecode()` (line 819)

```cpp
uint32_t num_matches = *num_matches_ptr;
// ...
if (entry->block_offset + data_offset + block_header->bytecode_size <= mapping_it->second.view_size)
```

**Problem:** `num_matches` is read from potentially untrusted cache data. No validation that `num_matches * sizeof(uint32_t)` fits within the block before reading match IDs.

**Fix:** Validate all reads against the mapped view size before dereferencing.

---

### 4. Error Recovery After Partial Writes

**Location:** `InsertSplitShaderToCache()` and `StoreSplitShaderRegexBytecode()`

If a write fails mid-operation (e.g., disk full), the index is not updated but the file may have partial data. This can corrupt the cache.

**Fix:** Use a write-ahead log or write to a temp file first, then atomically rename.

---

### 5. Block File Handle Leak on Error Path

**Location:** `GetBlockFileHandle()` (line 94-106)

If `CreateFileW` succeeds but we need to evict an LRU file and that close fails, the new file handle may be lost:

```cpp
HANDLE handle = CreateFileW(block_path, access, FILE_SHARE_READ, NULL,
                           creation, FILE_ATTRIBUTE_NORMAL, NULL);
if (handle == INVALID_HANDLE_VALUE) {
    LeaveCriticalSection(&cache->lock);
    return INVALID_HANDLE_VALUE;
}
```

**Fix:** Add error handling for the LRU eviction close, and ensure the new handle is properly stored or closed on any error path.

---

## Missing Features / Improvements

### 1. Configuration via INI

The cache settings are hardcoded or require code changes:
- `max_open_block_files` - Fixed at 10
- `pool_block_size` - Fixed at 64KB
- `max_pool_blocks` - Fixed at 100

**Recommendation:** Add INI options for these in `[Rendering]` section.

---

### 2. Cache Compression

The `BLOCK_FLAG_COMPRESSED` flag exists but is never used.

**Recommendation:** Implement optional LZ4/Zstd compression for block files to reduce disk space and I/O time.

---

### 3. Cache Coalescing

When shaders are invalidated, their space in block files is never reclaimed.

**Recommendation:** Implement periodic defragmentation or a "dead block" marker with background cleanup.

---

### 4. Async I/O

All file operations are synchronous, which can cause frame drops during cache writes.

**Recommendation:** Consider using `CreateFile` with `FILE_FLAG_OVERLAPPED` for write operations.

---

### 5. Backup/Rotate Feature

No built-in backup of the cache before updates.

**Recommendation:** Add a simple backup mechanism (keep last N versions) for cache recovery after corruption.

---

## Best Practices Recommendations

### 1. RAII for Resource Management

The current code manually manages handles and memory. Consider wrapping in RAII classes:

```cpp
class BlockFileHandle {
    HANDLE h;
public:
    ~BlockFileHandle() { if (h != INVALID_HANDLE_VALUE) CloseHandle(h); }
    operator HANDLE() { return h; }
};
```

---

### 2. Structured Logging

Replace raw `LogInfo` with structured logging or use the existing logging framework more consistently.

---

### 3. Unit Tests

No tests exist for the cache implementation. Add tests for:
- Index read/write round-trip
- Block file round-trip
- Migration correctness
- Concurrency scenarios
- Corruption detection

---

### 4. Documentation

The code lacks docstrings for public functions. Add documentation explaining:
- Thread safety guarantees
- Memory ownership contracts
- Cache format evolution strategy

---

### 5. Consider Modern C++ Patterns

- Use `std::unique_ptr` / `std::shared_ptr` for heap allocations
- Consider `std::optional` for nullable returns instead of NULL
- Use `std::string_view` / `std::wstring_view` for string parameters

---

## Summary of Priority Fixes

| Priority | Issue | Impact |
|----------|-------|--------|
| CRITICAL | Race condition in block file assignment | Data corruption |
| HIGH | LRU cache inconsistency | Memory/resource leaks |
| HIGH | Missing bounds validation on `num_matches` | Security vulnerability |
| MEDIUM | Memory pool fragmentation | Performance degradation |
| MEDIUM | No atomic writes | Cache corruption |
| MEDIUM | Memory mapping not tracked by LRU | Resource leaks |
| LOW | Missing configuration options | Usability |
| LOW | No compression support | Wasted disk space |
