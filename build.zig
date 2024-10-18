const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const exe = b.addExecutable(.{
        .target = target,
        .optimize = optimize,
        .name = "app_xrfw_sample",
    });
    b.installArtifact(exe);
    exe.entry = .disabled;
    exe.linkLibCpp();

    exe.addCSourceFiles(.{
        .files = &.{
            "app_xrfw_sample/main.cpp",
            //
            "src/impl/xrfw_impl_android_opengl_es.cpp",
            "src/impl/xrfw_impl_win32_d3d11.cpp",
            "src/impl/xrfw_impl_win32_opengl.cpp",
            "src/xrfw.cpp",
            // "src/xrfw_android.cpp",
            "src/xrfw_win32.cpp",
        },
        .flags = &.{
            "-DXR_USE_PLATFORM_WIN32",
            "-std=c++20",
            "-DXRFW_STATIC",
            "-DGLM_ENABLE_EXPERIMENTAL",
            "-DXR_USE_GRAPHICS_API_OPENGL",
        },
    });
    exe.addIncludePath(b.path("include"));

    const plog_dep = b.dependency("plog", .{});
    exe.addIncludePath(plog_dep.path("include"));

    const openxr_dep = b.dependency("openxr", .{});
    exe.addIncludePath(openxr_dep.path("include"));

    const glew_dep = b.dependency("glew", .{});
    exe.addIncludePath(glew_dep.path("include"));

    const glm_dep = b.dependency("glm", .{});
    exe.addIncludePath(glm_dep.path(""));

    const glfw_dep = b.dependency("glfw", .{});
    // std.debug.print("path {s}\n", .{glfw_dep.path("").getPath(b)});
    exe.addIncludePath(glfw_dep.path("include"));
}
