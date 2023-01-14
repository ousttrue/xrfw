common_lib = static_library('common', [
    'assertgl.c',
    'util_render2d.c',
    'util_shader.c',
    'util_debugstr.c',
    'util_matrix.cpp',
],
    c_args: ['-DXR_USE_PLATFORM_WIN32', '/wd4244'],
    dependencies: [glew_dep],
)
common_dep = declare_dependency(
    include_directories: include_directories('.'),
    link_with: common_lib,
)
