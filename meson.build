project('xrfw', 'cpp',
    version: '0.1.0',
)
xrfw_inc = include_directories('include')
xrfw_lib = library('xrfw', [
    'src/xrfw.cpp'
],
    install: true,
    include_directories: xrfw_inc,
    cpp_args: ['-DXRFW_BUILD'],
)
xrfw_dep = declare_dependency(
    include_directories: xrfw_inc,
    link_with: xrfw_lib,
)

subdir('examples')
