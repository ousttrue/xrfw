const std = @import("std");
const zcc = @import("compile_commands");

pub fn build(b: *std.Build) void {
    // make a list of targets that have include files and c source files
    var targets = std.ArrayList(*std.Build.Step.Compile).init(b.allocator);

    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const exe = b.addExecutable(.{
        .target = target,
        .optimize = optimize,
        .name = "app_xrfw_sample",
    });
    b.installArtifact(exe);
    targets.append(exe) catch @panic("OOM");

    exe.entry = .disabled;
    exe.linkLibCpp();

    exe.addCSourceFiles(.{
        .files = &.{
            "app_xrfw_sample/main.cpp",
            "app_xrfw_sample/ogldrawable.cpp",
            //
            // xrfw
            //
            "src/impl/xrfw_impl_android_opengl_es.cpp",
            "src/impl/xrfw_impl_win32_d3d11.cpp",
            "src/impl/xrfw_impl_win32_opengl.cpp",
            "src/xrfw.cpp",
            // "src/xrfw_android.cpp",
            "src/xrfw_win32.cpp",
        },
        .flags = &(flags ++ .{"-std=c++20"}),
    });
    exe.addIncludePath(b.path("include"));

    const plog_dep = b.dependency("plog", .{});
    exe.addIncludePath(plog_dep.path("include"));

    const openxr_dep = b.dependency("openxr", .{});
    exe.addIncludePath(openxr_dep.path("include"));
    exe.addLibraryPath(openxr_dep.path("native/x64/release/lib"));
    exe.linkSystemLibrary("openxr_loader");
    const install = b.addInstallBinFile(
        openxr_dep.path("native/x64/release/bin/openxr_loader.dll"),
        "openxr_loader.dll",
    );
    b.getInstallStep().dependOn(&install.step);

    const glew_dep = b.dependency("glew", .{});
    exe.addIncludePath(glew_dep.path("include"));
    exe.addCSourceFiles(.{
        .root = glew_dep.path("src"),
        .files = &.{
            "glew.c",
            // "glewinfo.c",
            // "visualinfo.c",
        },
        .flags = &flags,
    });

    const glm_dep = b.dependency("glm", .{});
    exe.addIncludePath(glm_dep.path(""));

    const glfw_dep = b.dependency("glfw", .{});
    exe.addIncludePath(glfw_dep.path("include"));
    exe.addCSourceFiles(.{
        .root = glfw_dep.path("src"),
        .files = &.{
            "context.c",
            "init.c",
            "input.c",
            "monitor.c",
            "platform.c",
            "vulkan.c",
            "window.c",
            "egl_context.c",
            "osmesa_context.c",
            "null_init.c",
            "null_monitor.c",
            "null_window.c",
            "null_joystick.c",
            //
            "win32_module.c",
            "win32_time.c",
            "win32_thread.c",
            "win32_init.c",
            "win32_joystick.c",
            "win32_monitor.c",
            "win32_window.c",
            "wgl_context.c",
        },
        .flags = &.{
            "-D_GLFW_WIN32",
        },
    });

    exe.linkSystemLibrary("OPENGL32");
    exe.linkSystemLibrary("GDI32");

    // std.debug.print("path {s}\n", .{glfw_dep.path("").getPath(b)});
    zcc.createStep(b, "zcc", targets.toOwnedSlice() catch @panic("OOM"));
}

const flags = .{
    "-DXRFW_STATIC",
    "-DGLM_ENABLE_EXPERIMENTAL",
    "-DGLEW_STATIC",
    "-DXR_USE_PLATFORM_WIN32",
    "-DXR_USE_GRAPHICS_API_OPENGL",
    // "-DXR_USE_GRAPHICS_API_D3D11",
};
