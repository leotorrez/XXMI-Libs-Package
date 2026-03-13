#!/usr/bin/env python3
"""
Shader Slot Profiling Filter Tool

Filters and annotates shader slot profiling output to show only configurations
containing specific texture resources.

Usage:
    python filter_shader_slots.py <profiling_file> <config_file>

Config file format (JSON):
{
    "textures": {
        "1bb147fa": "VarkaDiffuse",
        "b7ff7a6e": "VarkaNormalMap",
        "aaeadaab": "VarkaLightmap"
    },
    "filter_mode": "any"  // "any" = show configs with any tracked texture
                          // "all" = show configs with all tracked textures
                          // "only" = show configs with ONLY tracked textures
}
"""

import sys
import json
import re
from typing import Dict, List, Set, Optional
from dataclasses import dataclass


@dataclass
class SlotInfo:
    slot: int
    hash: str
    orig_hash: str
    stage: str  # "VS" or "PS"


@dataclass
class Configuration:
    draw_calls: int
    vs_slots: List[SlotInfo]
    ps_slots: List[SlotInfo]


@dataclass
class ShaderPair:
    vs_hash: str
    ps_hash: str
    total_draw_calls: int
    unique_configs: int
    configurations: List[Configuration]


@dataclass
class TrackedResource:
    hash: str
    associated_ibs: List[str]
    shader_pairs: List[ShaderPair]


class ShaderSlotProfileParser:
    def __init__(self, filepath: str):
        self.filepath = filepath
        self.resources: List[TrackedResource] = []
        
    def parse(self) -> List[TrackedResource]:
        with open(self.filepath, 'r') as f:
            lines = f.readlines()
        
        i = 0
        while i < len(lines):
            line = lines[i].strip()
            
            # Look for "Tracked Resource:"
            if line.startswith("Tracked Resource:"):
                resource = self._parse_resource(lines, i)
                if resource:
                    self.resources.append(resource)
                    # Skip to next resource
                    i = self._find_next_resource(lines, i + 1)
                    continue
            i += 1
        
        return self.resources
    
    def _parse_resource(self, lines: List[str], start_idx: int) -> Optional[TrackedResource]:
        # Parse tracked resource hash
        match = re.search(r'Tracked Resource: ([0-9a-f]+)', lines[start_idx])
        if not match:
            return None
        
        resource_hash = match.group(1)
        associated_ibs = []
        shader_pairs = []
        
        i = start_idx + 1
        
        # Parse associated IBs
        if i < len(lines) and lines[i].strip().startswith("Associated Index Buffers:"):
            ib_line = lines[i].strip()
            match = re.search(r'Associated Index Buffers: (.+)', ib_line)
            if match:
                associated_ibs = [ib.strip() for ib in match.group(1).split(',')]
            i += 1
        
        # Skip to "Shader Pairs"
        while i < len(lines) and not lines[i].strip().startswith("Shader Pairs"):
            i += 1
        i += 1  # Skip "Shader Pairs" line
        
        # Parse shader pairs
        while i < len(lines):
            line = lines[i].strip()
            
            # Check if we've reached the next resource
            if line.startswith("Tracked Resource:"):
                break
            
            # Look for VS/PS pair
            if line.startswith("VS:"):
                pair, i = self._parse_shader_pair(lines, i)
                if pair:
                    shader_pairs.append(pair)
                continue
            
            i += 1
        
        return TrackedResource(resource_hash, associated_ibs, shader_pairs)
    
    def _parse_shader_pair(self, lines: List[str], start_idx: int) -> tuple:
        # Parse VS and PS hashes
        match = re.search(r'VS: ([0-9a-f]+)\s+PS: ([0-9a-f]+)', lines[start_idx])
        if not match:
            return None, start_idx + 1
        
        vs_hash = match.group(1)
        ps_hash = match.group(2)
        
        i = start_idx + 1
        
        # Parse total draw calls
        total_draw_calls = 0
        if i < len(lines) and "Total Draw Calls:" in lines[i]:
            match = re.search(r'Total Draw Calls: (\d+)', lines[i])
            if match:
                total_draw_calls = int(match.group(1))
            i += 1
        
        # Parse unique configs count
        unique_configs = 0
        if i < len(lines) and "Unique Slot Configurations:" in lines[i]:
            match = re.search(r'Unique Slot Configurations: (\d+)', lines[i])
            if match:
                unique_configs = int(match.group(1))
            i += 1
        
        # Parse configurations
        configurations = []
        while i < len(lines):
            line = lines[i].strip()
            
            # Check if we've reached next shader pair or end
            if line.startswith("VS:") or line.startswith("Tracked Resource:"):
                break
            
            if line.startswith("Configuration (Draw calls:"):
                config, i = self._parse_configuration(lines, i)
                if config:
                    configurations.append(config)
                continue
            
            i += 1
        
        pair = ShaderPair(vs_hash, ps_hash, total_draw_calls, unique_configs, configurations)
        return pair, i
    
    def _parse_configuration(self, lines: List[str], start_idx: int) -> tuple:
        # Parse draw call count
        match = re.search(r'Configuration \(Draw calls: (\d+)\):', lines[start_idx])
        if not match:
            return None, start_idx + 1
        
        draw_calls = int(match.group(1))
        vs_slots = []
        ps_slots = []
        
        i = start_idx + 1
        current_stage = None
        
        while i < len(lines):
            line = lines[i].strip()
            
            # Check if we've reached next config
            if line.startswith("Configuration (Draw calls:") or line.startswith("VS:") or line.startswith("Tracked Resource:"):
                break
            
            # Check for stage markers
            if line == "VS Slots:":
                current_stage = "VS"
                i += 1
                continue
            elif line == "PS Slots:":
                current_stage = "PS"
                i += 1
                continue
            
            # Parse slot line: "t0: 1bb147fa (orig: 1bb147fa)"
            if current_stage and line.startswith("t"):
                match = re.search(r't(\d+): ([0-9a-f]+) \(orig: ([0-9a-f]+)\)', line)
                if match:
                    slot_info = SlotInfo(
                        int(match.group(1)),
                        match.group(2),
                        match.group(3),
                        current_stage
                    )
                    if current_stage == "VS":
                        vs_slots.append(slot_info)
                    else:
                        ps_slots.append(slot_info)
            
            # Empty line means end of config
            if not line:
                i += 1
                break
            
            i += 1
        
        config = Configuration(draw_calls, vs_slots, ps_slots)
        return config, i
    
    def _find_next_resource(self, lines: List[str], start_idx: int) -> int:
        for i in range(start_idx, len(lines)):
            if lines[i].strip().startswith("Tracked Resource:"):
                return i
        return len(lines)


class ShaderSlotFilter:
    def __init__(self, config: Dict):
        self.texture_names = config.get("textures", {})
        self.filter_mode = config.get("filter_mode", "any")
        self.tracked_hashes = set(self.texture_names.keys())
    
    def get_texture_name(self, hash_str: str) -> str:
        """Get the friendly name for a texture hash, or return the hash if unnamed."""
        return self.texture_names.get(hash_str, hash_str)
    
    def should_include_config(self, config: Configuration) -> bool:
        """Determine if a configuration should be included based on filter mode."""
        all_slots = config.vs_slots + config.ps_slots
        config_hashes = {slot.hash for slot in all_slots}
        
        if self.filter_mode == "any":
            # Include if ANY tracked texture is present
            return bool(config_hashes & self.tracked_hashes)
        elif self.filter_mode == "all":
            # Include if ALL tracked textures are present
            return self.tracked_hashes.issubset(config_hashes)
        elif self.filter_mode == "only":
            # Include if ONLY tracked textures are present
            return config_hashes.issubset(self.tracked_hashes)
        else:
            return True
    
    def annotate_output(self, resources: List[TrackedResource]) -> str:
        """Generate annotated and filtered output."""
        output = []
        output.append("Filtered Shader Slot Profiling Data")
        output.append("=" * 50)
        output.append(f"Filter mode: {self.filter_mode}")
        output.append(f"Tracked textures: {len(self.tracked_hashes)}")
        for hash_str, name in self.texture_names.items():
            output.append(f"  {hash_str}: {name}")
        output.append("")
        
        for resource in resources:
            output.append(f"Tracked Resource: {resource.hash}")
            output.append("-" * 50)
            
            if resource.associated_ibs:
                output.append(f"Associated Index Buffers: {', '.join(resource.associated_ibs)}")
                output.append("")
            
            # Filter shader pairs that have matching configs
            filtered_pairs = []
            for pair in resource.shader_pairs:
                filtered_configs = [c for c in pair.configurations if self.should_include_config(c)]
                if filtered_configs:
                    filtered_pairs.append((pair, filtered_configs))
            
            output.append(f"Shader Pairs (Total: {len(resource.shader_pairs)}, Filtered: {len(filtered_pairs)}):")
            output.append("")
            
            for pair, configs in filtered_pairs:
                output.append(f"  VS: {pair.vs_hash}  PS: {pair.ps_hash}")
                output.append(f"  Total Draw Calls: {pair.total_draw_calls}")
                output.append(f"  Matching Configurations: {len(configs)} / {pair.unique_configs}")
                output.append("")
                
                for config in configs:
                    output.append(f"    Configuration (Draw calls: {config.draw_calls}):")
                    
                    if config.vs_slots:
                        output.append("      VS Slots:")
                        for slot in config.vs_slots:
                            name = self.get_texture_name(slot.hash)
                            output.append(f"        t{slot.slot}: {slot.hash} [{name}] (orig: {slot.orig_hash})")
                    
                    if config.ps_slots:
                        output.append("      PS Slots:")
                        for slot in config.ps_slots:
                            name = self.get_texture_name(slot.hash)
                            output.append(f"        t{slot.slot}: {slot.hash} [{name}] (orig: {slot.orig_hash})")
                    
                    output.append("")
                
                output.append("")
            
            output.append("")
        
        return "\n".join(output)


def main():
    if len(sys.argv) < 3:
        print("Usage: python filter_shader_slots.py <profiling_file> <config_file>")
        print("\nExample config.json:")
        print(json.dumps({
            "textures": {
                "1bb147fa": "VarkaDiffuse",
                "b7ff7a6e": "VarkaNormalMap",
                "aaeadaab": "VarkaLightmap"
            },
            "filter_mode": "any"
        }, indent=2))
        sys.exit(1)
    
    profiling_file = sys.argv[1]
    config_file = sys.argv[2]
    
    # Load config
    with open(config_file, 'r') as f:
        config = json.load(f)
    
    # Parse profiling file
    print(f"Parsing {profiling_file}...")
    parser = ShaderSlotProfileParser(profiling_file)
    resources = parser.parse()
    print(f"Found {len(resources)} tracked resources")
    
    # Filter and annotate
    print("Filtering and annotating...")
    filter_tool = ShaderSlotFilter(config)
    output = filter_tool.annotate_output(resources)
    
    # Write output
    output_file = profiling_file.replace(".txt", "_filtered.txt")
    with open(output_file, 'w') as f:
        f.write(output)
    
    print(f"Filtered output written to: {output_file}")


if __name__ == "__main__":
    main()
