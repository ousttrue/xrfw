cpp_args = [
    '/wd4005',
    '-DXR_USE_PLATFORM_WIN32',
    '-DXR_USE_GRAPHICS_API_D3D11',
]
compiler = meson.get_compiler('cpp')
d3d11_dep = compiler.find_library('d3d11', required : false)
dxgi_dep = compiler.find_library('dxgi', required : false)
executable('BasicXrApp', [
    '../platform_win32_d3d11.cpp',
    'pch.cpp',
    'main.cpp',
    # 'OpenXrProgram.cpp',
    'DxUtility.cpp',
    'CubeGraphics.cpp',
],
    install: true,
    include_directories: include_directories('.'),
    cpp_args: cpp_args,
    dependencies: [openxr_loader_dep, plog_dep, xrfw_dep, dxgi_dep, d3d11_dep],
)
