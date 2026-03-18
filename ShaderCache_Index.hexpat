// ImHex Pattern for ShaderCache.idx (Split Shader Cache Index File)
// Author: Generated for XXMI-Libs-Package debugging
// Description: Pattern to parse the split shader cache index file
//
// USAGE:
//   1. Open ShaderCache.idx in ImHex
//   2. Load this pattern file (File -> Import -> Pattern)
//   3. The pattern will automatically parse and display cache information
//
// FILE LOCATION: <game_directory>/ShaderCache/ShaderCache.idx

#pragma endian little

import std.mem;
import std.io;
import std.sys;

// Color scheme for visual differentiation
#define COLOR_HEADER    0xFF4444FF  // Red
#define COLOR_ENTRY     0xFF44FF44  // Green
#define COLOR_METADATA  0xFFFF4444  // Blue
#define COLOR_RESERVED  0xFF888888  // Gray

// Shader type decoder
fn decode_shader_type(u32 encoded) {
    char result[5];
    result[0] = char((encoded >> 0) & 0xFF);
    result[1] = char((encoded >> 8) & 0xFF);
    result[2] = char((encoded >> 16) & 0xFF);
    result[3] = char((encoded >> 24) & 0xFF);
    result[4] = '\0';
    return result;
};

// Block flags decoder
fn decode_flags(u32 flags) {
    str result = "";
    if (flags & 0x00000001) result = result + "USED ";
    if (flags & 0x00000002) result = result + "HAS_ASM ";
    if (flags & 0x00000004) result = result + "COMPRESSED ";
    if (flags & 0x00000008) result = result + "REGEX_PATCH ";
    if (result == "") result = "NONE";
    return result;
};

// Index file header (64 bytes)
struct SplitCacheIndexHeader {
    char magic[8] [[color(COLOR_HEADER), comment("Should be '3DMSPLIT'")]];
    u32 version [[color(COLOR_HEADER), comment("Format version (4 = split cache)")]];
    u32 shader_count [[color(COLOR_HEADER), comment("Total shaders in cache")]];
    u32 index_count [[color(COLOR_HEADER), comment("Number of index entries")]];
    u32 shaders_per_block [[color(COLOR_HEADER), comment("Shaders per block file")]];
    u32 block_file_count [[color(COLOR_HEADER), comment("Number of .bin files")]];
    u32 shader_regex_hash [[color(COLOR_HEADER), comment("Hash of ShaderRegex config")]];
    u8 reserved[32] [[color(COLOR_RESERVED), comment("Reserved for future use")]];
} [[static]];

// Index entry (32 bytes per shader)
struct SplitCacheIndexEntry {
    u64 hash [[color(COLOR_ENTRY), comment("Shader hash")]];
    u32 type [[color(COLOR_ENTRY), transform("decode_shader_type"), comment("Shader type (vs/ps/cs/hs/ds/gs)")]];
    u32 bytecode_size [[color(COLOR_ENTRY), comment("Size of shader bytecode in bytes")]];
    u32 block_file_id [[color(COLOR_ENTRY), comment("Which ShaderCache_NNNN.bin file")]];
    u32 block_offset [[color(COLOR_ENTRY), comment("Offset within that block file")]];
    u32 flags [[color(COLOR_ENTRY), transform("decode_flags"), comment("Block flags")]];
    u32 reserved [[color(COLOR_RESERVED), comment("Reserved")]];
} [[static]];

// Main structure
struct ShaderCacheIndex {
    SplitCacheIndexHeader header [[comment("Index file header")]];
    
    // Index entries array
    SplitCacheIndexEntry entries[header.index_count] [[comment("Index entries for all cached shaders")]];
};

// Parse the file
ShaderCacheIndex cache_index @ 0x00;

// Display summary after parsing
std::print("====================================");
std::print("Split Shader Cache Index");
std::print("====================================");
std::print("Magic: {}", cache_index.header.magic);
std::print("Version: {}", cache_index.header.version);
std::print("Total Shaders: {}", cache_index.header.shader_count);
std::print("Index Entries: {}", cache_index.header.index_count);
std::print("Block Files: {}", cache_index.header.block_file_count);
std::print("Shaders per Block: {}", cache_index.header.shaders_per_block);
std::print("ShaderRegex Hash: 0x{:08X}", cache_index.header.shader_regex_hash);
std::print("====================================");
