glfw_dep = dependency('glfw3', 
    default_options: ['install=true'])
gl_dep = dependency('opengl')

executable('xrfw_sample', [
    'main.cpp'
],
    install: true,
    dependencies: [openxr_loader_dep, plog_dep, xrfw_dep, glfw_dep, gl_dep],
)
