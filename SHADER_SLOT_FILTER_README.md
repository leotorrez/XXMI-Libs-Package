# Shader Slot Profiling Filter Tool

A Python tool to filter and annotate shader slot profiling output from 3DMigoto, making it easy to track specific textures and understand where they're being used in shader pipelines.

## Features

- **Filter configurations** by specific texture hashes
- **Name your textures** (e.g., "VarkaDiffuse", "NormalMap") for easy identification
- **Multiple filter modes** to control how strict the filtering is
- **Annotated output** showing texture names inline with hashes
- **Statistics** showing how many configs match your filters

## Usage

### 1. Generate the profiling data

In-game:
1. Profile shader slots for your target resource (e.g., Index Buffer) in your INI:
   ```ini
   [TextureOverrideVarkaIB]
   hash = c4408c45
   profile_shader_slots = this
   ```
2. Run the game and trigger the draw calls
3. Press `Shift+F8` to dump the profiling data to `ShaderSlotProfiling.txt`

### 2. Create your filter config

Create a JSON config file (e.g., `my_filter.json`):

```json
{
  "textures": {
    "1bb147fa": "VarkaDiffuse",
    "b7ff7a6e": "VarkaNormalMap",
    "aaeadaab": "VarkaLightmap"
  },
  "filter_mode": "any"
}
```

### 3. Run the filter tool

```bash
python filter_shader_slots.py ShaderSlotProfiling.txt my_filter.json
```

This generates `ShaderSlotProfiling_filtered.txt` with only the relevant configurations.

## Filter Modes

### `"any"` (Default - Most Permissive)
Shows configurations containing **ANY** of your tracked textures.

**Use case**: "Show me all shader configurations that use VarkaDiffuse OR VarkaNormalMap"

**Example**: If you track Diffuse and NormalMap, you'll see:
- ✓ Configs with only Diffuse
- ✓ Configs with only NormalMap
- ✓ Configs with both Diffuse and NormalMap
- ✓ Configs with Diffuse + other textures

### `"all"` (Strict - Must Include All)
Shows configurations containing **ALL** of your tracked textures (plus possibly others).

**Use case**: "Show me only shader configurations that use BOTH VarkaDiffuse AND VarkaNormalMap together"

**Example**: If you track Diffuse and NormalMap, you'll see:
- ✗ Configs with only Diffuse
- ✗ Configs with only NormalMap
- ✓ Configs with both Diffuse and NormalMap
- ✓ Configs with Diffuse + NormalMap + other textures

### `"only"` (Most Restrictive)
Shows configurations containing **ONLY** your tracked textures (no other textures).

**Use case**: "Show me shader configurations that use exactly these textures and nothing else"

**Example**: If you track Diffuse and NormalMap, you'll see:
- ✗ Configs with only Diffuse
- ✗ Configs with only NormalMap
- ✓ Configs with exactly Diffuse and NormalMap
- ✗ Configs with Diffuse + NormalMap + other textures

## Output Format

The filtered output shows:

```
Filtered Shader Slot Profiling Data
==================================================
Filter mode: any
Tracked textures: 3
  1bb147fa: VarkaDiffuse
  b7ff7a6e: VarkaNormalMap
  aaeadaab: VarkaLightmap

Tracked Resource: c4408c45
--------------------------------------------------
Shader Pairs (Total: 23, Filtered: 5):

  VS: 0c0524e0d327e81e  PS: 02a30a81c0e37728
  Total Draw Calls: 2324
  Matching Configurations: 2 / 2

    Configuration (Draw calls: 1162):
      PS Slots:
        t0: 1bb147fa [VarkaDiffuse] (orig: 1bb147fa)
        t1: b7ff7a6e [VarkaNormalMap] (orig: b7ff7a6e)
```

Notice how texture names appear in brackets `[VarkaDiffuse]` next to their hashes!

## Use Cases

### Find where your diffuse texture is used
```json
{
  "textures": {
    "1bb147fa": "VarkaDiffuse"
  },
  "filter_mode": "any"
}
```

### Find shaders that use both diffuse and normal map
```json
{
  "textures": {
    "1bb147fa": "VarkaDiffuse",
    "b7ff7a6e": "VarkaNormalMap"
  },
  "filter_mode": "all"
}
```

### Find simple shaders that only use specific textures
```json
{
  "textures": {
    "1bb147fa": "VarkaDiffuse",
    "b7ff7a6e": "VarkaNormalMap"
  },
  "filter_mode": "only"
}
```

## Tips

1. **Start with "any" mode** to see all usages of your textures
2. **Name unknown textures** by their hash at first, then update names as you identify them
3. **Look at draw call counts** to see which configurations are used most frequently
4. **Compare PS slot assignments** across different shader pairs to understand LOD changes
5. **Use "only" mode** to find the simplest/cleanest shader configurations

## Requirements

- Python 3.6+
- No external dependencies (uses only standard library)

## Example Workflow

1. Dump profiling data from the game
2. Run the tool with "any" mode to see all configs
3. Identify the most common configurations
4. Update texture names in your config
5. Run again with "all" mode to see specific combinations
6. Use the filtered output to understand your shader pipeline

This helps you understand:
- Which shaders use which textures
- What texture slots (t0, t1, t2...) each texture binds to
- How LODs affect texture usage
- Which configurations are most common
