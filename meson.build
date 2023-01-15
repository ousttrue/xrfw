project('xrfw', ['cpp', 'c'],
    version: '0.1.0',
    default_options: ['cpp_std=c++20'],
)

compiler = meson.get_compiler('cpp')
message(compiler.get_id())

openxr_loader_dep = dependency('openxr_loader_windows_prebuilt')
plog_dep = dependency('plog')

cpp_args = ['-DXRFW_BUILD']
if compiler.get_id() == 'msvc'
    cpp_args += ['-D_CRT_SECURE_NO_WARNINGS']
endif
if host_machine.system() == 'windows'
    cpp_args += [
        '-DXR_USE_PLATFORM_WIN32', 
        '-DXR_USE_GRAPHICS_API_OPENGL',
        '-DXR_USE_GRAPHICS_API_D3D11',
        '-DPLOG_EXPORT',
    ]
else
endif

xrfw_inc = include_directories('include')
xrfw_lib = library('xrfw', [
    'src/xrfw.cpp',
    'src/xrfw_win32.cpp',
],
    install: true,
    include_directories: xrfw_inc,
    cpp_args: cpp_args,
    dependencies: [plog_dep, openxr_loader_dep],
)
xrfw_dep = declare_dependency(
    include_directories: xrfw_inc,
    link_with: xrfw_lib,
    dependencies: [plog_dep, openxr_loader_dep],
)

glfw_dep = dependency('glfw3', 
    default_options: ['install=true'])
# gl_dep = dependency('opengl')
glew_dep = dependency('glew', 
    default_options:['default_library=static'],
)
glm_dep = dependency('glm')

if get_option('impl_opengl')
    xrfw_impl_opengl_deps = [
        xrfw_dep, glm_dep, glfw_dep, glew_dep,
    ]
    xrfw_impl_opengl_args = [
        '-DXR_USE_PLATFORM_WIN32',
        '-DXR_USE_GRAPHICS_API_OPENGL',
    ]
    xrfw_impl_opengl_lib = static_library('xrfw_impl_opengl', [
        'src/impl/xrfw_impl_win32_opengl.cpp',
    ],
        cpp_args: xrfw_impl_opengl_args,
        dependencies: xrfw_impl_opengl_deps,
    )
    xrfw_impl_opengl_dep = declare_dependency(
        link_with: xrfw_impl_opengl_lib,
        dependencies: xrfw_impl_opengl_deps,
        compile_args: xrfw_impl_opengl_args,
    )
    subdir('thirdparty/common')
    subdir('app_gl2gridOXR')
    subdir('app_xrfw_sample')
endif
if get_option('impl_d3d11')
    d3d11_dep = compiler.find_library('d3d11', required : false)
    dxgi_dep = compiler.find_library('dxgi', required : false)
    xrfw_impl_d3d11_deps = [
        xrfw_dep, dxgi_dep, d3d11_dep, 
    ]
    xrfw_impl_d3d11_args = [
        '-DXR_USE_PLATFORM_WIN32',
        '-DXR_USE_GRAPHICS_API_D3D11',
    ]
    xrfw_impl_d3d11_lib = static_library('xrfw_impl_d3d11', [
        'src/impl/xrfw_impl_win32_d3d11.cpp',
    ],
        cpp_args: xrfw_impl_d3d11_args,
        dependencies: xrfw_impl_d3d11_deps,
    )
    xrfw_impl_d3d11_dep = declare_dependency(
        link_with: xrfw_impl_d3d11_lib,
        dependencies: xrfw_impl_d3d11_deps,
        compile_args: xrfw_impl_d3d11_args,
    )
    subdir('app_BasicXrApp')
endif
if get_option('tests')
    subdir('tests')
endif

meson.override_dependency('xrfw', xrfw_dep)
