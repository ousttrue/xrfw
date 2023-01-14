cpp_args = [
    '/wd4005',
]
compiler = meson.get_compiler('cpp')
d3d11_dep = compiler.find_library('d3d11', required : false)
dxgi_dep = compiler.find_library('dxgi', required : false)
executable('BasicXrApp', [
    'pch.cpp',
    'App.cpp',
    'OpenXrProgram.cpp',
    'DxUtility.cpp',
    'CubeGraphics.cpp',
],
    gui_app : true,
    install: true,
    include_directories: include_directories('.'),
    cpp_args: cpp_args,
    dependencies: [openxr_loader_dep, plog_dep, xrfw_dep, dxgi_dep, d3d11_dep],
)
