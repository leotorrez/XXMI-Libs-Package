// ImHex Pattern for ShaderCache_NNNN.bin (Split Shader Cache Block Files)
// Author: Generated for XXMI-Libs-Package debugging
// Description: Pattern to parse individual shader block files containing bytecode
//
// USAGE:
//   1. Open any ShaderCache_NNNN.bin file in ImHex (e.g., ShaderCache_0000.bin)
//   2. Load this pattern file (File -> Import -> Pattern)
//   3. The pattern will automatically parse all shader blocks in the file
//
// FILE LOCATION: <game_directory>/ShaderCache/ShaderCache_NNNN.bin (where NNNN is 0000-9999)

#pragma endian little

import std.mem;
import std.io;
import std.sys;

// Color scheme
#define COLOR_HEADER    0xFF4444FF  // Red
#define COLOR_BLOCK     0xFFFF4444  // Blue
#define COLOR_METADATA  0xFFFFAA44  // Orange
#define COLOR_BYTECODE  0xFF44FF44  // Green
#define COLOR_PADDING   0xFF888888  // Gray

// Block flags
#define BLOCK_FLAG_USED 0x00000001
#define BLOCK_FLAG_HAS_ASM 0x00000002
#define BLOCK_FLAG_COMPRESSED 0x00000004
#define BLOCK_FLAG_REGEX_PATCH 0x00000008

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
    if (flags & BLOCK_FLAG_USED) result = result + "USED ";
    if (flags & BLOCK_FLAG_HAS_ASM) result = result + "HAS_ASM ";
    if (flags & BLOCK_FLAG_COMPRESSED) result = result + "COMPRESSED ";
    if (flags & BLOCK_FLAG_REGEX_PATCH) result = result + "REGEX_PATCH ";
    if (result == "") result = "NONE";
    return result;
};

// Block file header (32 bytes)
struct SplitCacheBlockFileHeader {
    char magic[8] [[color(COLOR_HEADER), comment("Should be '3DMBLOCK'")]];
    u32 version [[color(COLOR_HEADER), comment("Format version")]];
    u32 block_id [[color(COLOR_HEADER), comment("Block file ID (NNNN)")]];
    u32 shader_count [[color(COLOR_HEADER), comment("Number of shaders in this file")]];
    u32 data_size [[color(COLOR_HEADER), comment("Total data size in bytes")]];
    u8 reserved[8] [[color(COLOR_HEADER), comment("Reserved")]];
} [[static]];

// Regex metadata for patched shaders
struct RegexMetadata {
    u32 num_matches [[color(COLOR_METADATA), comment("Number of ShaderRegex matches")]];
    if (num_matches > 0) {
        u32 match_ids[num_matches] [[color(COLOR_METADATA), comment("IDs of matched ShaderRegex patterns")]];
    }
} [[static]];

// Shader block header (24 bytes)
struct ShaderBlockHeader {
    u32 magic [[color(COLOR_BLOCK), comment("0x53444342 = 'SDCB'")]];
    u32 flags [[color(COLOR_BLOCK), transform("decode_flags"), comment("Block flags")]];
    u64 shader_hash [[color(COLOR_BLOCK), comment("Shader hash")]];
    u32 shader_type [[color(COLOR_BLOCK), transform("decode_shader_type"), comment("Shader type")]];
    u32 bytecode_size [[color(COLOR_BLOCK), comment("Bytecode size in bytes")]];
    u32 total_size [[color(COLOR_BLOCK), comment("Total block size (4-byte aligned)")]];
} [[static]];

// Complete shader block with data
struct ShaderBlock {
    ShaderBlockHeader header [[comment("Block header")]];
    
    // Regex metadata (if REGEX_PATCH flag is set)
    if (header.flags & BLOCK_FLAG_REGEX_PATCH) {
        RegexMetadata regex_meta [[comment("ShaderRegex match information")]];
    }
    
    // Shader bytecode
    u8 bytecode[header.bytecode_size] [[color(COLOR_BYTECODE), comment("Compiled shader bytecode (DXBC)")]];
    
    // Calculate padding based on total_size from header
    // total_size includes: header + metadata + bytecode + padding (4-byte aligned)
    u32 consumed_size = sizeof(header);
    if (header.flags & BLOCK_FLAG_REGEX_PATCH) {
        consumed_size = consumed_size + sizeof(regex_meta);
    }
    consumed_size = consumed_size + header.bytecode_size;
    
    // Padding to reach total_size (4-byte aligned)
    u32 padding_size = header.total_size - consumed_size;
    if (padding_size > 0 && padding_size < 4) {
        u8 padding[padding_size] [[color(COLOR_PADDING), comment("4-byte alignment padding")]];
    }
};

// Main structure for block file
struct ShaderCacheBlockFile {
    SplitCacheBlockFileHeader header [[comment("Block file header")]];
    
    // Parse shader blocks (dynamically based on count)
    ShaderBlock shaders[header.shader_count] [[comment("Shader blocks in this file")]];
};

// Parse the file
ShaderCacheBlockFile block_file @ 0x00;

// Display summary after parsing
std::print("====================================");
std::print("Split Shader Cache Block File");
std::print("====================================");
std::print("Magic: {}", block_file.header.magic);
std::print("Version: {}", block_file.header.version);
std::print("Block ID: {}", block_file.header.block_id);
std::print("Shader Count: {}", block_file.header.shader_count);
std::print("Data Size: {} bytes ({} KB)", block_file.header.data_size, block_file.header.data_size / 1024);
std::print("====================================");
std::print("Parsed {} shaders successfully", block_file.header.shader_count);
