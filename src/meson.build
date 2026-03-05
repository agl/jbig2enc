if not cxx.has_header('leptonica/allheaders.h')
    # On some platforms the .pc file points inside the `leptonica` directory.
    # This is also true for the Wrap fallback.
    # Just generate some wrapper headers for that case.
    foreach h : ['allheaders.h', 'pix_internal.h', 'array_internal.h']
        configure_file(
            input: meson.project_source_root() / 'meson-support' / 'leptonica-header.h.in',
            output: h,
            build_subdir: 'leptonica',
            configuration: {'header': h},
        )
    endforeach
endif

lib_src = files(
    'jbig2arith.cc',
    'jbig2comparator.cc',
    'jbig2enc.cc',
    'jbig2sym.cc',
)

if cxx.get_id() == 'msvc'
    # TODO: Need to mark symbol visibility in header files.
    # Meson can handle it for us:
    # https://mesonbuild.com/Snippets-module.html#symbol_visibility_header
    lib = static_library(
        'jbig2enc',
        lib_src,
        install: true,
        dependencies: dependencies,
    )
else
    lib = library(
        'jbig2enc',
        lib_src,
        soversion: shlib_version,
        install: true,
        dependencies: dependencies,
    )
endif

install_headers(
    'jbig2arith.h',
    'jbig2comparator.h',
    'jbig2segments.h',
    'jbig2structs.h',
    'jbig2sym.h',
)

exe = executable(
    'jbig2',
    'jbig2.cc',
    install: true,
    dependencies: dependencies,
    link_with: lib,
)

test(
    'basic',
    exe,
    args: [
        '-s',
        '-a',
        '-v',
        meson.project_source_root() / 'images' / 'feyn.tif',
    ],
)

pkg = import('pkgconfig')
pkg.generate(
    lib,
    description: 'JBIG2 Encoder',
    version: version_clean,
    license: meson.project_license()[0],
    url: 'https://github.com/agl/jbig2enc',
)

# This makes the library usable as Meson subprorect.
lib_dep = declare_dependency(include_directories: '.', link_with: lib)
meson.override_dependency('jbig2enc', lib_dep)
