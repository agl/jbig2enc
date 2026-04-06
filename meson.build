project(
    'jbig2enc',
    'cpp',
    license: 'Apache-2.0',
    license_files: ['COPYING'],
    version: run_command('meson-support'/'version.sh', 'get-vcs', check: true).stdout().strip(),
    meson_version: '>= 1.10.0',
    default_options: {
        'cpp_std': 'c++14',
        'warning_level': '3',
        'b_ndebug': 'if-release',
        'buildtype': 'debugoptimized',
        'b_lto': true,
        'b_pie': true,
        # Leptonica Wrap fallback options
        'leptonica:warning_level': '0',
        'leptonica:tests': 'disabled',
        'leptonica:progs': 'disabled',
        'leptonica:libjpeg': 'enabled',
        'leptonica:libtiff': 'enabled',
        'leptonica:libz': 'enabled',
        'libjpeg-turbo:tests': 'disabled',
        'libpng:tests': false,
        'tiff:jpeg': 'enabled',
        'tiff:zlib': 'enabled',
        'leptonica:default_library': 'static',
        'libjpeg-turbo:default_library': 'static',
        'libpng:default_library': 'static',
        'libtiff:default_library': 'static',
        'libz:default_library': 'static',
        'libjpeg-turbo:warning_level': '0',
        'libpng:warning_level': '0',
        'libtiff:warning_level': '0',
        'libz:warning_level': '0',
    },
)
# Cement version for dist tarballs so that git is not required.
meson.add_dist_script('meson-support'/'version.sh', 'set-dist', meson.project_version())

version_clean = meson.project_version().split('-')[0]
version_parts = version_clean.split('.')
# Match libtool versioning scheme.
# This can be independent from the project version.
so_current = version_parts[0].to_int()  # increment for all API changes
so_revision = version_parts[1].to_int()  # always increment
so_age = version_parts.get(2, '0').to_int()  # increment only for backwards-compatible API changes
shlib_version = '@0@.@1@.@2@'.format(so_current - so_age, so_age, so_revision)

cxx = meson.get_compiler('cpp')

add_project_arguments(
    '-DVERSION="@0@"'.format(meson.project_version()),
    language: 'cpp',
)

lept_dep = dependency('lept', version: '>=1.74')
dependencies = [
    lept_dep,
    cxx.find_library('m', required: false),
]

if host_machine.system() == 'windows'
    dependencies += cxx.find_library('ws2_32')
    add_project_arguments('-DWIN32', '-D_USE_MATH_DEFINES', language: 'cpp')
    if cxx.get_id() == 'msvc'
        add_project_link_arguments('setargv.obj', language: 'cpp')
        add_project_arguments(
            '-D_CRT_SECURE_NO_WARNINGS',
            '-D_SCL_SECURE_NO_WARNINGS',
            language: ['c', 'cpp'],
        )
    endif
endif

subdir('doc')
subdir('src')

install_data(
    'jbig2topdf.py',
    install_dir: get_option('bindir'),
    install_tag: 'runtime',
)
