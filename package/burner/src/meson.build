project('burner', 'c',
    default_options: [
        'c_std=gnu11',
        'optimization=2',
        'warning_level=2',
        'werror=true',
        'b_lto=true',
        'b_pie=true',
    ])

add_project_arguments(
    '-Wno-sign-compare',
    language: 'c')

libevdev = dependency('libevdev')

executable('burner',
    'burner.c',
    'leds.c',
    dependencies: [libevdev],
    gnu_symbol_visibility: 'hidden',
    install: true)

executable('is-any-button-pressed',
    'is-any-button-pressed.c',
    dependencies: [libevdev],
    gnu_symbol_visibility: 'hidden',
    install: true)

install_data('burner-slot-handler',
    install_mode: 'rwxr-xr-x',
    install_dir: get_option('bindir'))
