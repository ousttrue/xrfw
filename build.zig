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
    // std.debug.print("path {s}\n", .{glfw_dep.path("").getPath(b)});
    exe.addIncludePath(glfw_dep.path("include"));

    exe.linkSystemLibrary("OpenGL32");

    zcc.createStep(b, "zcc", targets.toOwnedSlice() catch @panic("OOM"));
}

const flags = .{
    "-DXR_USE_PLATFORM_WIN32",
    "-DXRFW_STATIC",
    "-DGLM_ENABLE_EXPERIMENTAL",
    "-DXR_USE_GRAPHICS_API_OPENGL",
    "-DGLEW_STATIC",
};
