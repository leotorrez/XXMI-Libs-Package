#pragma once

#include <stdint.h>
#include <unordered_map>
#include <vector>
#include <windows.h>

// Split Shader Cache System
// Separates index from shader data for faster partial updates
// 
// File structure:
//   ShaderCache.idx     - Header + Index table (always loaded, lightweight)
//   ShaderCache_NNN.bin - Shader blocks in chunks (loaded on demand)
//
// Benefits:
//   - Only rebuild changed shaders, not entire cache
//   - Faster startup (index is smaller)
//   - Parallel block file access
//   - Easier backup/sync (only changed blocks)

// Constants
#define SHADER_CACHE_SPLIT_MAGIC "3DMSPLIT"
#define SHADER_CACHE_SPLIT_VERSION 4 // Version 4: Split cache format
#define SHADERS_PER_BLOCK_FILE 100   // Number of shaders per block file

// Block flags
#define BLOCK_FLAG_USED 0x00000001
#define BLOCK_FLAG_HAS_ASM 0x00000002
#define BLOCK_FLAG_COMPRESSED 0x00000004
#define BLOCK_FLAG_REGEX_PATCH 0x00000008

// Alignment macros
#define ALIGN_4(x) (((x) + 3) & ~3)
#define ALIGN_16(x) (((x) + 15) & ~15)

// Index file header (64 bytes)
#pragma pack(push, 4)
struct SplitCacheIndexHeader {
  char magic[8];              // "3DMSPLIT"
  uint32_t version;           // Format version (4 = split cache)
  uint32_t shader_count;      // Total shaders in cache
  uint32_t index_count;       // Entries in index
  uint32_t shaders_per_block; // Shaders per block file
  uint32_t block_file_count;  // Number of block files
  uint32_t shader_regex_hash; // Hash of all ShaderRegex sections
  uint8_t reserved[32];       // Reserved for future use
};

// Index entry (32 bytes) - extends original with block file info
struct SplitCacheIndexEntry {
  uint64_t hash;           // Shader hash
  uint32_t type;           // Encoded shader type
  uint32_t bytecode_size;  // Size of bytecode
  uint32_t block_file_id;  // Which block file contains this shader
  uint32_t block_offset;   // Offset within that block file
  uint32_t flags;          // Block flags
  uint32_t reserved;       // Reserved for future use
};

// Block file header (32 bytes) - header for each ShaderCache_NNN.bin file
struct SplitCacheBlockFileHeader {
  char magic[8];        // "3DMBLOCK"
  uint32_t version;     // Format version
  uint32_t block_id;    // This block file's ID
  uint32_t shader_count; // Number of shaders in this file
  uint32_t data_size;   // Total data size in bytes
  uint8_t reserved[8];  // Reserved
};

// Block header (24 bytes, 4-byte aligned)
// For regex-patched shaders (BLOCK_FLAG_REGEX_PATCH set):
//   Followed by: [RegexMetadata][Bytecode]
//   RegexMetadata: uint32_t num_matches, uint32_t match_ids[num_matches]
// For original shaders:
//   Followed by: [Bytecode]
struct ShaderBlockHeader {
  uint32_t magic;         // 0x53444342 = "SDCB" (Shader Data Cache Block)
  uint32_t flags;         // BLOCK_FLAG_* flags
  uint64_t shader_hash;   // Shader hash
  uint32_t shader_type;   // Encoded type: "ps"=0x7073, "vs"=0x7673, etc.
  uint32_t bytecode_size; // Size of bytecode data (after metadata if regex)
  uint32_t total_size;    // Total size of this block (header + metadata + data,
                          // 4-byte aligned)
};
#pragma pack(pop)

// In-memory cache state
struct SplitShaderCache {
  // Index file handle
  HANDLE index_file_handle;
  wchar_t index_path[MAX_PATH];
  wchar_t block_dir[MAX_PATH]; // Directory for block files
  
  // Cache header
  SplitCacheIndexHeader header;
  
  // Index structures
  std::vector<SplitCacheIndexEntry> index;
  std::unordered_map<uint64_t, size_t> hash_map; // Maps key to index in vector
  
  // Open block file handles (LRU cache)
  std::unordered_map<uint32_t, HANDLE> open_block_files;
  std::vector<uint32_t> block_file_lru; // Most recently used block files
  uint32_t max_open_block_files;        // Max cached file handles (default: 10)
  
  // Memory-mapped file support for fast read access
  struct BlockMapping {
    HANDLE file_mapping;  // File mapping handle
    void *view;           // Mapped view pointer
    SIZE_T view_size;     // Size of mapped view
  };
  std::unordered_map<uint32_t, BlockMapping> block_mappings;
  bool use_memory_mapping;  // Enable memory-mapped I/O (default: true)
  
  // Memory pool for bytecode allocations
  struct MemoryBlock {
    uint8_t *data;
    uint32_t size;
    bool in_use;
  };
  std::vector<MemoryBlock> memory_pool;
  uint32_t pool_block_size;           // Size of each pool block (default: 64KB)
  uint32_t max_pool_blocks;           // Max blocks in pool (default: 100)
  bool use_memory_pool;               // Enable memory pool (default: true)
  
  // State tracking
  bool dirty;                   // Index needs flush
  bool read_only;              // Operating in read-only mode
  
  // Thread safety
  CRITICAL_SECTION lock;
  
  // Statistics
  uint64_t query_count;
  uint64_t insert_count;
  uint64_t hit_count;
  uint64_t miss_count;
  uint64_t block_file_opens; // How many times we opened block files
  uint64_t mmap_reads;       // Reads using memory-mapped I/O
  uint64_t file_reads;       // Reads using traditional file I/O
  uint64_t pool_allocs;      // Allocations from memory pool
  uint64_t heap_allocs;      // Allocations from heap (fallback)
};

// API Functions

// Initialize split shader cache from directory
// cache_dir: Directory containing ShaderCache.idx and block files
// Creates directory structure if doesn't exist
SplitShaderCache *InitSplitShaderCache(const wchar_t *cache_dir, 
                                       uint32_t regex_hash = 0,
                                       uint32_t shaders_per_block = SHADERS_PER_BLOCK_FILE);

// Close cache and flush to disk
void CloseSplitShaderCache(SplitShaderCache *cache);

// Query shader bytecode from cache
// Returns allocated bytecode buffer (caller must free with FreeSplitShaderBytecode)
// Returns NULL if not found
const void *QuerySplitShaderBytecode(SplitShaderCache *cache, 
                                     uint64_t hash,
                                     const wchar_t *type, 
                                     uint32_t *out_size);

// Free bytecode buffer allocated by Query functions
void FreeSplitShaderBytecode(SplitShaderCache *cache, const void *bytecode);

// Insert or update shader in cache
// Automatically assigns to appropriate block file
bool InsertSplitShaderToCache(SplitShaderCache *cache, 
                               uint64_t hash, 
                               const wchar_t *type,
                               const void *bytecode, 
                               uint32_t bytecode_size,
                               FILETIME timestamp);

// Query regex-patched shader
const void *QuerySplitShaderRegexBytecode(SplitShaderCache *cache,
                                          uint64_t hash, 
                                          const wchar_t *type,
                                          uint32_t expected_regex_hash, 
                                          uint32_t *out_size,
                                          uint32_t *out_num_matches, 
                                          uint32_t **out_match_ids);

// Store regex-patched shader
bool StoreSplitShaderRegexBytecode(SplitShaderCache *cache, 
                                   uint64_t hash,
                                   const wchar_t *type, 
                                   const void *bytecode,
                                   uint32_t bytecode_size, 
                                   uint32_t num_matches,
                                   const uint32_t *match_ids);

// Flush index to disk (fast operation, only writes index file)
bool FlushSplitCacheIndex(SplitShaderCache *cache);

// Get cache statistics
void GetSplitCacheStatistics(SplitShaderCache *cache, 
                             uint32_t *out_shader_count,
                             uint32_t *out_block_file_count);

// Log detailed cache statistics
void LogSplitCacheStatistics(SplitShaderCache *cache);

// Mark shader as regex-processed (sets BLOCK_FLAG_REGEX_PATCH on existing entry)
bool MarkSplitShaderRegexProcessed(SplitShaderCache *cache,
                                   uint64_t hash,
                                   const wchar_t *type);

// Validate cache integrity
uint32_t ValidateSplitCacheIntegrity(SplitShaderCache *cache);

// Migration: Convert old monolithic cache to split format
bool MigrateMonolithicToSplit(const wchar_t *old_cache_path, 
                               const wchar_t *new_cache_dir);

// Utility functions for shader type encoding
uint32_t EncodeShaderType(const wchar_t *type);
void DecodeShaderType(uint32_t encoded, wchar_t *out_type, size_t out_size);
