project('xrfw', 'cpp',
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
    cpp_args += ['-DXR_USE_PLATFORM_WIN32=1', '-DXR_USE_GRAPHICS_API_OPENGL=1']
else
endif

xrfw_inc = include_directories('include')
xrfw_lib = library('xrfw', [
    'src/xrfw.cpp'
],
    install: true,
    include_directories: xrfw_inc,
    cpp_args: cpp_args,
    dependencies: [plog_dep, openxr_loader_dep],
)
xrfw_dep = declare_dependency(
    include_directories: xrfw_inc,
    link_with: xrfw_lib,
)

subdir('examples')
