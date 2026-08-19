# unit tests

test_common_src = common_src + embed_lib_src + files(
    'ap/test_lua_player.cpp',
    'ap/test_lua_seed.cpp',
    'appupdater/test_parser.cpp',
    'core/singleton2.cpp',
    'core/test_autotracker.cpp',
    'core/test_fs.cpp',
    'core/test_jsonitem_inherit_codes.cpp',
    'core/test_jsonitem_provides.cpp',
    'core/test_jsonitem_quantity.cpp',
    'core/test_jsonitem_state.cpp',
    'core/test_jsonitem_toggle_badged.cpp',
    'core/test_locations_parser.cpp',
    'core/test_luaitem.cpp',
    'core/test_pack.cpp',
    'core/test_sha256.cpp',
    'core/test_singleton.cpp',
    'core/test_taskqueue.cpp',
    'core/test_tracker.cpp',
    'core/test_unicode_paths.cpp',
    'core/test_util.cpp',
    'core/test_version.cpp',
    'core/test_zip.cpp',
    'gh/api/test_release.cpp',
    'gh/api/test_releaseasset.cpp',
    'gh/api/test_releases.cpp',
    'http/test_http.cpp',
    'http/test_http_cache.cpp',
    'jsonutil/test_commasplit.cpp',
    'lib/test_lua.cpp',
    'ui/test_item_render.cpp',
    'ui/test_packimagefuture.cpp',
    'ui/test_trackerview_makeitem.cpp',
    'uilib/test_assetimagefuture.cpp',
    'uilib/test_dlg.cpp',
    'uilib/test_imagefilter.cpp',
    'uilib/test_img.cpp',
    'uilib/test_tabs.cpp',
    'uilib/test_textutil.cpp',
    'uilib/test_widget_color.cpp',
    # Note: GUI test files are always compiled to show them as "skipped", but need WITH_GUI_TESTS define to run.
    'uilib/test_gui_progressbar.cpp',
)

test_exe_src = test_common_src + files('main.cpp')

test_include_dirs = []

# option handling

test_exe_cpp_args = []

if get_option('gui-tests')
    test_exe_cpp_args += ['-DWITH_GUI_TESTS']
endif

# platform and architecture specific configuration

if host_machine.system() == 'windows'
    # Windows-only tests
    test_exe_src += files(
        'mock/test_gmock_win32.cpp',
        'os/test_winapi.cpp',
    )
    # Extra windows test dependencioes
    # TODO: provide gmock-win32 dependency from lib subdir meson.build or add a subproject
    test_include_dirs += [include_directories(
        '../lib/gmock-win32/include',
    )]
    test_exe_src += files(
        '../lib/gmock-win32/src/gmock-win32.cpp',
    )
endif

# test dependencies

gtest_dep = dependency('gtest', required : false)  # only for tests
gmock_dep = dependency('gmock', required : false)  # only for tests
if not gtest_dep.found() or not gmock_dep.found()
    # TODO: use a wrap if not found?
    warning('Test dependencies not found! Won\'t be able to build/run tests!')
endif

# test runner target

test_exe_include_dirs = main_exe_include_dirs + test_include_dirs

test_exe_deps = common_deps + [gtest_dep, gmock_dep]

test_exe = executable(
    'poptracker-test',
    sources : test_exe_src,
    dependencies : test_exe_deps,
    include_directories : test_exe_include_dirs,
    override_options : override_options,
    build_by_default : false,
    cpp_args : test_exe_cpp_args,
)

test(
    'test',
    test_exe,
    workdir : meson.current_source_dir() + '/..',
    protocol : 'gtest',
)

# fuzzer

fuzzer_exe = executable(
    'fuzzer',
    sources : test_common_src + files('fuzz.cpp'),
    dependencies : test_exe_deps,
    include_directories : test_exe_include_dirs,
    override_options : override_options,
    build_by_default : false,
    cpp_args : ['-fsanitize=fuzzer,address', '-DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION'],
    link_args : ['-fsanitize=fuzzer,address'],
)
