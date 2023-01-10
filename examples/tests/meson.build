
catch2_with_main_dep = dependency('catch2-with-main')

executable('math_test', [
    'math_test.cpp',
],
    install: true,
    dependencies: [catch2_with_main_dep, openxr_loader_dep, glm_dep],
)
