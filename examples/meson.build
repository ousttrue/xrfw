glfw_dep = dependency('glfw3', 
    default_options: ['install=true'])
gl_dep = dependency('opengl')

cpp_args = ['-DPLOG_IMPORT']
executable('xrfw_sample', [
    'main.cpp',
    'oxrsession.cpp',
    'oxrrenderer.cpp',
],
    install: true,
    cpp_args: cpp_args,
    dependencies: [openxr_loader_dep, plog_dep, xrfw_dep, glfw_dep, gl_dep],
)
