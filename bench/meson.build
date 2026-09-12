# benchmarks

bench_exe_src = common_src + embed_lib_src + files(
    'main.cpp',
    'core/bench_signal.cpp',
    'lib/bench_lua.cpp',
    'uilib/bench_colorhelper.cpp',
)

# target

bench_exe_include_dirs = main_exe_include_dirs
bench_exe_deps = common_deps + [sltbench_dep]

bench_exe = executable(
    'poptracker-benchmark',
    sources : bench_exe_src,
    dependencies : bench_exe_deps,
    include_directories : bench_exe_include_dirs,
    override_options : override_options,
    build_by_default : false,
)

benchmark(
    'bench',
    bench_exe,
    workdir : meson.current_source_dir() + '/..',
)
