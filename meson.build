# Beware: we'll probably do some awful stuff because we want to control compiler features for the default builds.

# use as
# `meson setup build`
# `meson compile -C build`
# For debugging, the recommended config is
# `meson setup --reconfigure --buildtype debugoptimized -Doptimization=g build`

# NOTE: default_options are applied when setting up the project.
#       Changing them here later does nothing, not even with --reconfigure. Use rm -r instead.
#       See https://github.com/mesonbuild/meson/issues/2193
#       Should we consider executable(..., override_options : ['cpp_std=c++1z']) ?

# NOTE: meson may use ccache. To clear ccache, run ccache -C

# NOTE: we actually compile the .c files as cpp, but meson requires us to specify 'c' anyway.

project(
    'PopTracker',
    ['cpp', 'c'],
    version : '0.35.4',
    default_options : [
        'warning_level=2',  # We don't want pedantic. We target specific compilers.
        'cpp_std=c++1z',
        'werror=true',
        'buildtype=release',  # mirror old Makefile behavior: default to release
    ],
    license : 'GPL-3.0-only',
    # license_files : [  # requires meson_version : '>=1.1.0'
    #    'LICENSE',
    #    'PLUGIN LICENSE ADDENDUM.md',
    #    'asset-licenses.txt',
    #],
    meson_version : '>=0.60.0',
)

# TODO: Makefile feature parity:
# TODO:   dist (maybe scripts + a custom run target for easy discovery?)
# TODO:   build common_src only once (into shared dependency?) when possible

# TODO: Other stuff
# TODO:   update build and release github action workflows

# TODO: New stuff
# TODO:   vcs tag
# TODO:   write back-compat Makefile
# TODO:   -Wshadow, -Wconversion
# TODO:   ccache in CI
# TODO:   -ftrapv in fuzzer?
# TODO:   switch to meson dist for _full-source.tar.xz ?

static_exe = false  # TODO: add a build option?
use_boost_filesystem = false  # TODO: add a build option?
build_type = get_option('buildtype')
override_options = []
cpp_compiler = meson.get_compiler('cpp')
linker = cpp_compiler.get_linker_id()
if linker not in ['ld.bfd', 'ld.gold', 'ld64'] and not linker.endswith('.lld')
    message('Using unsupported linker: @0@'.format(linker))  # some features may not work
endif

# source files

common_src = files(
    'src/poptracker.cpp',
    'src/ap/aptracker.cpp',
    'src/ap/archipelago.cpp',
    'src/appupdater/appupdater.cpp',
    'src/core/assets.cpp',
    'src/core/autotracker.cpp',
    'src/core/baseitem.cpp',
    'src/core/gameinfo.cpp',
    'src/core/imagereference.cpp',
    'src/core/jsonitem.cpp',
    'src/core/layoutclass.cpp',
    'src/core/layoutnode.cpp',
    'src/core/location.cpp',
    'src/core/locationsection.cpp',
    'src/core/log.cpp',
    'src/core/luaitem.cpp',
    'src/core/map.cpp',
    'src/core/pack.cpp',
    'src/core/scripthost.cpp',
    'src/core/sha256.cpp',
    'src/core/statemanager.cpp',
    'src/core/tracker.cpp',
    'src/core/version.cpp',
    'src/core/zip.cpp',
    'src/gh/api/release.cpp',
    'src/gh/api/releaseasset.cpp',
    'src/http/http.cpp',
    'src/http/httpcache.cpp',
    'src/http/httputil.cpp',
    'src/luaconnector/connection.cpp',
    'src/luaconnector/luaconnector.cpp',
    'src/luaconnector/server.cpp',
    'src/luasandbox/luapackio.cpp',
    'src/packmanager/packmanager.cpp',
    'src/uat/uatclient.cpp',
    'src/ui/broadcastwindow.cpp',
    'src/ui/defaulttrackerwindow.cpp',
    'src/ui/item.cpp',
    'src/ui/loadpackwidget.cpp',
    'src/ui/maptooltip.cpp',
    'src/ui/mapwidget.cpp',
    'src/ui/settingswindow.cpp',
    'src/ui/trackerview.cpp',
    'src/ui/trackerwindow.cpp',
    'src/uilib/button.cpp',
    'src/uilib/dlg.cpp',
    'src/uilib/dock.cpp',
    'src/uilib/drawhelper.cpp',
    'src/uilib/fontstore.cpp',
    'src/uilib/group.cpp',
    'src/uilib/image.cpp',
    'src/uilib/imagebutton.cpp',
    'src/uilib/imagefilter.cpp',
    'src/uilib/label.cpp',
    'src/uilib/progressbar.cpp',
    'src/uilib/tabs.cpp',
    'src/uilib/texturemanager.cpp',
    'src/uilib/timer.cpp',
    'src/uilib/tooltip.cpp',
    'src/uilib/ui.cpp',
    'src/uilib/widget.cpp',
    'src/uilib/window.cpp',
    'src/usb2snes/usb2snes.cpp',
)

common_include_dirs = [include_directories(
   'lib',
   'lib/lua',
   'lib/asio/include',
   'lib/miniz',
   'lib/json/include',
   'lib/valijson/include',
   'lib/tinyfiledialogs',
   'lib/wswrap/include',
   'lib/fmt/include',
)]

# lib sources that are compiled as part of the app (rather than used as a lib)
# TODO: replace those with separate build files or wraps to increase flexibility?
embed_lib_src = files(
    'lib/fmt/src/format.cc',
)

main_exe_src = common_src + embed_lib_src + files([
  'src/main.cpp',
  'src/app.cpp',
])

# defines

add_project_arguments('-DASIO_STANDALONE', '-DASIO_HEADER_ONLY', language : ['c', 'cpp'])
add_project_arguments('-DJSON_HAS_CPP_17', language : ['c', 'cpp'])  # ignore detection. we don't support anything older
add_project_arguments('-DLUA_CPP', language : ['c', 'cpp'])  # we compile lua as cpp instead of c

# strip dead code

add_project_arguments('-ffunction-sections', '-fdata-sections', language: ['c', 'cpp'])
add_project_link_arguments(
    cpp_compiler.get_supported_link_arguments('-Wl,-dead_strip', '-Wl,--gc-sections'),
    language : ['c', 'cpp']
)

# hardening

add_project_arguments(
    '-fno-common',
    '-fstack-protector-strong',
    '-fpic',
    '-fPIE',
    language : ['c', 'cpp']
)
add_project_arguments(
    cpp_compiler.get_supported_arguments('-fstack-clash-protection'),
    language: ['c', 'cpp']
)
add_project_link_arguments('-fstack-protector-strong', language : ['c', 'cpp'])

# disable some warnings to allow -Werror

add_project_arguments(
    '-Wno-unused-function',
    '-Wnon-virtual-dtor',
    '-Wno-deprecated-declarations',
    '-Wno-null-pointer-subtraction',
    '-Wno-shift-count-overflow',
    language : 'cpp')  # TODO: fix those warnings if possible
add_project_link_arguments(
    '-Wno-stringop-overflow',  # false positives with GCC in lua and fmt
    '-Wno-odr',  # unsure if ODR violations are real or not
    '-Wno-lto-type-mismatch',  # as above
    language : ['c', 'cpp']
)

if build_type == 'debug' or build_type == 'debugoptimized'
    # improve debugging experience
    add_project_arguments(
        '-fno-omit-frame-pointer',
        '-DLUA_USE_APICHECK',
        '-DLUAI_ASSERT',
        language : ['c', 'cpp']
    )
    add_project_link_arguments(
        '-fno-omit-frame-pointer',
        language : ['c', 'cpp']
    )
else
    # remove debug-only code and symbols
    override_options += ['b_ndebug=true', 'strip=true']
endif

if build_type != 'debug'
    # performance-concious build type
    override_options += ['b_lto=true']
    # It looks like b_lto=true does not add optimization level to the linker, so we do it ourselves.
    # FIXME: we are probably missing another switch? Or is this a bug?
    optimization_level = get_option('optimization')
    if optimization_level in ['1', '2', '3', 'g', 's', 'z']
        add_project_link_arguments('-O' + optimization_level, language : ['c', 'cpp'])
    endif
endif

# figure out the base version a.b.c and the extra, which is either empty, -something or +something

version_full = meson.project_version()
version_parts_dash = version_full.split('-')
version_parts_plus = version_full.split('+')
assert(version_parts_dash.length() < 3 and version_parts_plus.length() < 3, 'Expected at most 1 - and 1 + in version')
if version_parts_dash.length() == 1 and version_parts_plus.length() == 1
    # version has neither - nor +
    version_no_extra = version_full
    version_extra = ''
    version_prerelease_flag = '0'
elif version_parts_plus.length() == 1 or version_parts_plus[0].startswith(version_parts_dash[0])
    # version has - after numbers
    version_no_extra = version_parts_dash[0]
    version_extra = '-' + version_parts_dash[1]
    version_prerelease_flag = '1'  # semver does not have -post1
elif version_parts_dash.length() == 1 or version_parts_dash[0].startswith(version_parts_plus[0])
    # version has + after numbers
    version_no_extra = version_parts_plus[0]
    version_extra = '+' + version_parts_plus[1]
    version_prerelease_flag = '0'
else
    assert(false, 'Should be unreachable')
endif
version_number_parts = version_no_extra.split('.')
assert(version_number_parts.length() == 3, 'Expected version to be a.b.c[-pre][+patch], found @0@'.format(version_full))

# write version.h

version_data = configuration_data()
version_data.set('VERSION_MAJOR', version_number_parts[0])
version_data.set('VERSION_MINOR', version_number_parts[1])
version_data.set('VERSION_REVISION', version_number_parts[2])
version_data.set('VERSION_EXTRA', version_extra)
version_data.set('VERSION_PRERELEASE_FLAG', version_prerelease_flag)
versions_h = configure_file(
    input : 'src/version.h.in',
    output : 'version.h',
    configuration : version_data,
    install : false,
)
common_include_dirs += [include_directories('.')]  # why does this work?!

# platform and architecture specific configuration

if host_machine.system() == 'windows'
    add_project_arguments('-D_WIN32_WINNT=0x0601', '-DWIN32_LEAN_AND_MEAN', language : ['c', 'cpp'])
    add_project_link_arguments('-mwindows', language : ['c', 'cpp'])
    static_exe = true  # TODO: only if undefined (not set via option)?
    if meson.version().version_compare('>=1.3.0')
        if not cpp_compiler.has_define('_WIN32')
            message('Using wrong compiler for win32? _WIN32 is undefined.')
        endif
    endif
elif host_machine.system() == 'darwin'
    brew_prefix = run_command('brew', '--prefix', check : true).stdout().strip()
    message('Using brew prefix: @0@'.format(brew_prefix))
    deplyment_target = get_option('macos-deplyment-target')
    add_project_arguments('-mmacosx-version-min=' + deplyment_target, language : ['c', 'cpp'])
    if deplyment_target.version_compare('<10.15')
        use_boost_filesystem = true  # TODO: only if undefined (not set via option)?
    endif

    # Use `brew link --overwrite openssl@...` to use a specific OpenSSL version.
    # This replaces the explicit -I and -L we passed in the old Makefile.
endif

if static_exe
    add_project_link_arguments('-static', '-static-pie', language : ['c', 'cpp'])
endif

if use_boost_filesystem
    message('Using boost::filesystem')
    add_project_arguments('-DNO_STD_FILESYSTEM', language : ['c', 'cpp'])
    add_project_link_arguments('-lboost_filesystem', language : ['c', 'cpp'])
endif

# get definition of in-tree-defined dependencies (lua_dep, sltbench_dep)

subdir('lib')

# system dependencies

zlib_dep = dependency('zlib', static : static_exe)
ssl_dep = dependency('openssl', static : static_exe)

sdl_dep = dependency('sdl2', static : static_exe)
sdl_ttf_dep = dependency('sdl2_ttf', static : static_exe, required : false)
if not sdl_ttf_dep.found()
    # TODO: use wrap/subproject if not found via pkg-config (or just always use wrap)?
    sdl_ttf_dep = [
        cpp_compiler.find_library('SDL2_ttf', required : true),
        cpp_compiler.find_library('rpcrt4', required : false),  # only required on Windows
    ]
endif
sdl_image_dep = dependency('sdl2_image', static : static_exe, required : false)
if not sdl_image_dep.found()
    # TODO: use wrap/subproject if not found via pkg-config (or just always use wrap)?
    # NOTE: default SDL2_image links webp, but webp libdir might be different for static builds
    extra_lib_dir = meson.get_external_property('extra_lib_dir', [])
    sdl_image_dep = [
        cpp_compiler.find_library('SDL2_image', required : true),
        cpp_compiler.find_library('webpdemux', dirs : extra_lib_dir),
        cpp_compiler.find_library('webp', dirs : extra_lib_dir),
    ]
endif

thread_dep = dependency('threads')

# dependency groups

common_deps = [lua_dep, zlib_dep, ssl_dep, sdl_dep, sdl_ttf_dep, sdl_image_dep, thread_dep]

# platform-specific dependencies

if host_machine.system() == 'windows'
    wsock_dep = cpp_compiler.find_library('wsock32', required : true)
    ws2_dep = cpp_compiler.find_library('ws2_32', required : true)
    crypt_dep = cpp_compiler.find_library('crypt32', required : true)
    common_deps += [wsock_dep, ws2_dep, crypt_dep]
endif

# main executable

main_exe_name = meson.project_name().to_lower()
main_exe_description = meson.project_name()

main_exe_include_dirs = common_include_dirs

main_exe_link_args = []

if host_machine.system() == 'windows'
    windows = import('windows')

    main_exe_manifest = configure_file(
        input : 'win32/exe.manifest.template.xml',
        output : 'poptracker.exe.manifest',
        configuration :  configuration_data({
            'VERSION' : version_no_extra + '.0',  # Windows wants #.#.#.# format
            'NAME' : main_exe_name,
            'DESCRIPTION' : main_exe_description,
        }),
    )

    if meson.version().version_compare('>=1.4.0')
        app_rc_args = ['-DMANIFEST_PATH="@0@"'.format(main_exe_manifest.full_path()) ]
    else
        app_rc_args = ['-DMANIFEST_PATH="@0@"'.format(main_exe_manifest) ]
    endif
    main_exe_res = windows.compile_resources(
        files('src/app.rc'),  # TODO: move to /win32/
        args : app_rc_args,
        depend_files : files(
            'src/version.h.in',
            'assets/icon.ico',
        ) + main_exe_manifest,
        include_directories : include_directories('.'),
    )

    main_exe_src += main_exe_res
endif

main_exe = executable(
    main_exe_name,
    sources : main_exe_src,
    dependencies : common_deps,
    include_directories : main_exe_include_dirs,
    link_args : main_exe_link_args,
    override_options : override_options,
    win_subsystem: 'windows',
    install: true,
)

# data

if get_option('datadir').endswith('/share')
    # assume datadir is shared/unix-like and append app name
    data_dir = get_option('datadir') / 'poptracker'
else
    # otherwise assume we package the app in a portable way
    data_dir = get_option('datadir')
endif

# on macos, we currently have 2 different data folders, because assets needs to be next to the binary
resources_dir = data_dir

install_data('LICENSE', install_dir : data_dir)
install_data('CHANGELOG.md', install_dir : data_dir)
install_data('README.md', install_dir : data_dir)
install_data('CREDITS.md', install_dir : data_dir)
install_subdir('assets', install_dir : data_dir)
install_subdir('schema', install_dir : resources_dir)
install_subdir('api', install_dir : resources_dir)
install_subdir('key', install_dir : resources_dir)
install_emptydir(data_dir / 'packs')  # TODO: skip this on macos?  # FIXME: is this correct?

# run
# run with `meson compile -C build run`
# This is for running from terminal, not from IDE.
# For IDE, configure the run options (i.e. working dir) for the main_exe (poptracker).

run_target(
    'run',
    command : [files('scripts/run.sh'), main_exe],  # TODO: .bat or .ps1 if we ever support MSVC
)

# benchmarks
# run with `meson test --benchmark -v -C build`

subdir('bench')

# tests
# run with `meson test -C build`

subdir('test')  # unit tests

test(
    'version',  # this tests being able to run the built binary
    main_exe,
    args : '--version',  # TODO: check that output matches the project version
    workdir : meson.current_source_dir() + '/..',
    # verbose : true,  # always print the version; requires meson >= 0.62. Do this once we drop Ubuntu 22.04.
)

test(
    'list-packs',  # this tests http
    main_exe,
    args : '--list-packs',
    workdir : meson.current_source_dir() + '/..',
)

if host_machine.system() == 'windows'
    # try to print VersionInfo
    # TODO: check that it matches the version
    powershell = find_program('powershell', required : false)  # only for test
    if powershell.found()
        test(
            'VersionInfo',
            powershell,
            args : [
                '-NoLogo',
                '-NoProfile',
                '-Command',
                '(Get-Item -Path "@0@").VersionInfo | Format-List -Force'.format(main_exe),
            ],
            # verbose : true,  # always print the version; requires meson >= 0.62. Do this once we drop Ubuntu 22.04.
            depends : main_exe,
        )
    endif
endif

# update cacert
# run with `meson compile -C build update-cacert` or run the script directly

run_target(
    'update-cacert',
    command : files('scripts/update-cacert.sh'),
)

# docs
# doc/commandline.txt can be generated by running main_exe --help and swapping out the printed exe name
# run with `meson compile -C build commandline` or run the script directly

run_target(
    'commandline',
    command : [files('scripts/update-commandline.sh'), main_exe],
    depends : main_exe,
)
