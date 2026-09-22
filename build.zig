const std = @import("std");

const pcre2_static_srcs = &[_][]const u8{
    "pcre2-10.30/src/pcre2_auto_possess.c",
    "pcre2-10.30/src/pcre2_compile.c",
    "pcre2-10.30/src/pcre2_config.c",
    "pcre2-10.30/src/pcre2_context.c",
    "pcre2-10.30/src/pcre2_convert.c",
    "pcre2-10.30/src/pcre2_dfa_match.c",
    "pcre2-10.30/src/pcre2_error.c",
    "pcre2-10.30/src/pcre2_find_bracket.c",
    "pcre2-10.30/src/pcre2_jit_compile.c",
    "pcre2-10.30/src/pcre2_maketables.c",
    "pcre2-10.30/src/pcre2_match_data.c",
    "pcre2-10.30/src/pcre2_match.c",
    "pcre2-10.30/src/pcre2_newline.c",
    "pcre2-10.30/src/pcre2_ord2utf.c",
    "pcre2-10.30/src/pcre2_pattern_info.c",
    "pcre2-10.30/src/pcre2_serialize.c",
    "pcre2-10.30/src/pcre2_string_utils.c",
    "pcre2-10.30/src/pcre2_study.c",
    "pcre2-10.30/src/pcre2_substitute.c",
    "pcre2-10.30/src/pcre2_substring.c",
    "pcre2-10.30/src/pcre2_tables.c",
    "pcre2-10.30/src/pcre2_ucd.c",
    "pcre2-10.30/src/pcre2_valid_utf.c",
    "pcre2-10.30/src/pcre2_xclass.c",
};

const directxtk_srcs = &[_][]const u8{
    "DirectXTK/Src/SpriteBatch.cpp",
    "DirectXTK/Src/SpriteFont.cpp",
    "DirectXTK/Src/CommonStates.cpp",
    "DirectXTK/Src/SimpleMath.cpp",
    "DirectXTK/Src/DDSTextureLoader.cpp",
    "DirectXTK/Src/WICTextureLoader.cpp",
    "DirectXTK/Src/ScreenGrab.cpp",
    "DirectXTK/Src/BinaryReader.cpp",
    "DirectXTK/Src/GraphicsMemory.cpp",
    "DirectXTK/Src/BasicEffect.cpp",
    "DirectXTK/Src/PrimitiveBatch.cpp",
    "DirectXTK/Src/VertexTypes.cpp",
    "DirectXTK/Src/EffectCommon.cpp",
};

const d3d11_srcs = &[_][]const u8{
    "DirectX11/CommandList.cpp",
    "DirectX11/cursor.cpp",
    "DirectX11/D3D11Wrapper.cpp",
    "DirectX11/DLLMainHook.cpp",
    "DirectX11/FrameAnalysis.cpp",
    "DirectX11/HackerContext.cpp",
    "DirectX11/HackerDevice.cpp",
    "DirectX11/HackerDXGI.cpp",
    "DirectX11/HackerInputLayout.cpp",
    "DirectX11/HookedContext.cpp",
    "DirectX11/HookedDevice.cpp",
    "DirectX11/HookedDXGI.cpp",
    "DirectX11/Hunting.cpp",
    "DirectX11/IniHandler.cpp",
    "DirectX11/Input.cpp",
    "DirectX11/lock.cpp",
    "DirectX11/Overlay.cpp",
    "DirectX11/Override.cpp",
    "DirectX11/profiling.cpp",
    "DirectX11/ResourceHash.cpp",
    "DirectX11/ShaderRegex.cpp",
    "DirectX11/ByteCodeReader.cpp",
    "iid.cpp",
    "ini_parser_lite.cpp",
    "util.cpp",
    "crc32c-hw-1.0.5/src/crc32c.cpp",
    "D3D_Shaders/Assembler.cpp",
    "D3D_Shaders/SignatureParser.cpp",
    "HLSLDecompiler/DecompileHLSL.cpp",
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{
        .default_target = .{ .os_tag = .windows, .abi = .gnu },
    });
    const optimize = b.standardOptimizeOption(.{
        .preferred_optimize_mode = .ReleaseFast,
    });

    const arch = target.result.cpu.arch;
    const is_x64 = arch == .x86_64;
    if (!is_x64 and arch != .x86) {
        @panic("only x86 and x86_64 windows targets are supported");
    }
    const pcre2_config_dir = if (is_x64) "pcre2-10.30/build64" else "pcre2-10.30/build32";
    const nektra_lib = if (is_x64) "Nektra/NktHookLib64.lib" else "Nektra/NktHookLib.lib";

    const pcre2_write = b.addWriteFiles();
    const pcre2_chartables = pcre2_write.addCopyFile(b.path("pcre2-10.30/src/pcre2_chartables.c.dist"), "pcre2_chartables.c");
    const pcre2_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    pcre2_mod.addIncludePath(b.path("pcre2-10.30/src"));
    pcre2_mod.addIncludePath(b.path(pcre2_config_dir));
    pcre2_mod.addCMacro("PCRE2_CODE_UNIT_WIDTH", "8");
    pcre2_mod.addCMacro("PCRE2_STATIC", "1");
    pcre2_mod.addCMacro("HAVE_CONFIG_H", "1");
    pcre2_mod.addCSourceFiles(.{
        .root = b.path("."),
        .files = pcre2_static_srcs,
    });
    pcre2_mod.addCSourceFiles(.{
        .root = pcre2_chartables.dirname(),
        .files = &.{"pcre2_chartables.c"},
    });
    const pcre2_lib = b.addLibrary(.{
        .name = "pcre2-8",
        .root_module = pcre2_mod,
    });

    const directxtk_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .link_libcpp = true,
    });
    directxtk_mod.addIncludePath(b.path("DirectXTK/Inc"));
    directxtk_mod.addIncludePath(b.path("DirectXTK/Src"));
    directxtk_mod.addIncludePath(b.path("DirectXMath"));
    directxtk_mod.addCMacro("_WIN7_PLATFORM_UPDATE", "1");
    directxtk_mod.addCMacro("WIN32", "1");
    directxtk_mod.addCMacro("NDEBUG", "1");
    directxtk_mod.addCMacro("_LIB", "1");
    directxtk_mod.addCMacro("UNICODE", "1");
    directxtk_mod.addCMacro("_UNICODE", "1");
    directxtk_mod.addCSourceFiles(.{
        .root = b.path("."),
        .files = directxtk_srcs,
    });
    const directxtk_lib = b.addLibrary(.{
        .name = "DirectXTK",
        .root_module = directxtk_mod,
    });

    const binarydecompiler_srcs = &[_][]const u8{
        "BinaryDecompiler/decode.cpp",
        "BinaryDecompiler/decodeDX9.cpp",
        "BinaryDecompiler/reflect.cpp",
    };

    const binarydecompiler_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .link_libcpp = true,
    });
    binarydecompiler_mod.addIncludePath(b.path("."));
    binarydecompiler_mod.addIncludePath(b.path("BinaryDecompiler"));
    binarydecompiler_mod.addIncludePath(b.path("BinaryDecompiler/include"));
    binarydecompiler_mod.addCSourceFiles(.{
        .root = b.path("."),
        .files = binarydecompiler_srcs,
        .flags = &.{
            "-Wno-write-strings",
            "-Wno-format",
            "-std=gnu++17",
        },
    });
    const binarydecompiler_lib = b.addLibrary(.{
        .name = "BinaryDecompiler",
        .root_module = binarydecompiler_mod,
    });

    const d3d11_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .link_libcpp = true,
        .pic = true,
    });
    const d3d11_include_dirs = [_][]const u8{
        ".",
        "DirectX11",
        "HLSLDecompiler",
        "D3D_Shaders",
        "DirectXTK/Inc",
        "DirectXTK/Src",
        "DirectXMath",
        "Nektra",
        "pcre2-10.30/src",
        "crc32c-hw-1.0.5/include",
        "BinaryDecompiler/include",
        "BinaryDecompiler",
    };
    for (d3d11_include_dirs) |dir| d3d11_mod.addIncludePath(b.path(dir));
    d3d11_mod.addIncludePath(b.path(pcre2_config_dir));
    const d3d11_defines = [_][2][]const u8{
        .{ "UNICODE", "1" },
        .{ "_UNICODE", "1" },
        .{ "_WINDOWS", "1" },
        .{ "_USRDLL", "1" },
        .{ "NDEBUG", "1" },
        .{ "MIGOTO_DX", "11" },
        .{ "CRC32C_STATIC", "1" },
        .{ "PCRE2_STATIC", "1" },
        .{ "PCRE2_CODE_UNIT_WIDTH", "8" },
        .{ "COM_STDMETHOD_CAN_THROW", "1" },
        .{ "_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES", "1" },
        .{ "_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT", "1" },
        .{ "_SH_DENYNO", "0x40" },
        .{ "__in", "" },
        .{ "__in_opt", "" },
        .{ "__in_z", "" },
        .{ "__in_z_opt", "" },
        .{ "__inout", "" },
        .{ "__inout_opt", "" },
        .{ "__inout_z_opt", "" },
        .{ "__out", "" },
        .{ "__out_opt", "" },
        .{ "__out_z", "" },
        .{ "__out_z_opt", "" },
        .{ "__in_bcount(a)", "" },
        .{ "__in_bcount_opt(a)", "" },
        .{ "__in_ecount(a)", "" },
        .{ "__in_ecount_opt(a)", "" },
        .{ "__in_range(a, b)", "" },
        .{ "__in_xcount_opt(a)", "" },
        .{ "__out_bcount(a)", "" },
        .{ "__out_bcount_opt(a)", "" },
        .{ "__out_ecount(a)", "" },
        .{ "__out_ecount_opt(a)", "" },
        .{ "__out_ecount_part_opt(a, b)", "" },
    };
    for (d3d11_defines) |def| d3d11_mod.addCMacro(def[0], def[1]);
    d3d11_mod.addCSourceFiles(.{
        .root = b.path("."),
        .files = d3d11_srcs,
        .flags = &.{
            "-Wno-write-strings",
            "-std=gnu++17",
            "-fms-extensions",
            "-fno-operator-names",
            "-Wno-nonportable-include-path",
            "-Wno-format",
            "-Wno-microsoft-exception-spec",
            "-Wno-inconsistent-missing-override",
            "-Wno-switch",
            "-Wno-deprecated-declarations",
            "-Wno-empty-body",
            "-Wno-trigraphs",
            "-Wno-nontrivial-memaccess",
            "-Wno-parentheses",
            "-Wno-delete-abstract-non-virtual-dtor",
        },
    });
    d3d11_mod.addCSourceFiles(.{
        .root = b.path("."),
        .files = &.{"DirectX11/HookAddresses.c"},
        .flags = &.{ "-Wno-format" },
    });
    d3d11_mod.addWin32ResourceFile(.{
        .file = b.path("DirectX11/DirectX11.rc"),
        .include_paths = &.{
            b.path("."),
            b.path("DirectX11"),
        },
    });

    d3d11_mod.linkLibrary(pcre2_lib);
    d3d11_mod.linkLibrary(directxtk_lib);
    d3d11_mod.linkLibrary(binarydecompiler_lib);
    d3d11_mod.addLibraryPath(b.path(".pixi/envs/default/Library/lib/zig/libc/mingw/lib-common"));
    if (is_x64) {
        // NktHookLib64.lib is compiled with the MSVC C++ ABI; this shim provides
        // the Itanium-mangled names this module's sources reference and forwards
        // to the MSVC implementations.  The lib's .drectve directives also ask
        // for uuid.lib, so expose mingw's libuuid.a via the library search path.
        d3d11_mod.addCSourceFiles(.{
            .root = b.path("."),
            .files = &.{"Nektra/NktHookLib_gnu_shim.cpp"},
            .flags = &.{ "-std=gnu++17", "-Wno-write-strings" },
        });
        d3d11_mod.addLibraryPath(b.path(".pixi/envs/default/Library/lib/zig/libc/mingw/lib64"));
    }
    d3d11_mod.addObjectFile(b.path(nektra_lib));
    const system_libs = [_][]const u8{
        "gdi32",
        "dxgi",
        "shlwapi",
        "dbghelp",
        "windowscodecs",
        "ole32",
        "oleaut32",
        "user32",
        "shell32",
        "uuid",
        "XINPUT9_1_0",
        "d3dcompiler_47",
    };
    for (system_libs) |lib| d3d11_mod.linkSystemLibrary(lib, .{});

    const d3d11 = b.addLibrary(.{
        .linkage = .dynamic,
        .name = "d3d11",
        .root_module = d3d11_mod,
        .win32_module_definition = b.path("DirectX11/d3d11Wrapper.def"),
    });

    b.installArtifact(d3d11);
}