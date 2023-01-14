cpp_args = [
    '-DWINDOWS_LEAN_AND_MEAN',
    '-DPLOG_IMPORT', 
    '-DXR_USE_GRAPHICS_API_OPENGL=1',
    '-DNOMINMAX',
]
executable('gl2gridOXR', [
    'main.cpp',
    'render_scene.cpp',
    '../platform_win32_glfw_opengl.cpp',    
],
    install: true,
    cpp_args: cpp_args,
    dependencies: [openxr_loader_dep, plog_dep, xrfw_dep, glfw_dep, glew_dep, glm_dep, common_dep],
)
