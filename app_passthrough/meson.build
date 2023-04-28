screencapture_dep = dependency('screencapture')
executable(
    'app_passthrough',
    [
        'main.cpp',
        'desktop_capture.cpp',
    ],
    install: true,
    dependencies: [
        xrfw_impl_d3d11_dep,
        cuber_dep,
        screencapture_dep,
    ],
)
