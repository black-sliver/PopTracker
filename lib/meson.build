# define some global variables from lib/

# our custom liblua
# TODO: write a meson build file for this and move to subprojects/?

lua_inc = include_directories('lua')

lua_core_src = files(
    'lua/lapi.c',
    'lua/lcode.c',
    'lua/lctype.c',
    'lua/ldebug.c',
    'lua/ldo.c',
    'lua/ldump.c',
    'lua/lfunc.c',
    'lua/lgc.c',
    'lua/llex.c',
    'lua/lmem.c',
    'lua/lobject.c',
    'lua/lopcodes.c',
    'lua/lparser.c',
    'lua/lstate.c',
    'lua/lstring.c',
    'lua/ltable.c',
    'lua/ltm.c',
    'lua/lundump.c',
    'lua/lvm.c',
    'lua/lzio.c',
    'lua/ltests.c',
)

lua_aux_src = files(
    'lua/lauxlib.c',
)

lua_lib_src = files(
    'lua/lbaselib.c',
    'lua/lcorolib.c',  # FIXME: currently unsupported and not loaded
    'lua/lmathlib.c',
    'lua/ltablib.c',
    'lua/lstrlib.c',
    'lua/lutf8lib.c',
    # skip building risky libs:
    'lua/loslib.c',  # TODO: replace this with a custom lib. We currently modify os after loading.
    #'lua/ldblib.c',  # TODO: make this optional for debugging purposes?
    #'lua/liolib.c',
    #'lua/loadlib.c',
    # we can't use the "load all" helper:
    #'lua/linit.c',
)
lua_lib_src = lua_core_src + lua_aux_src + lua_lib_src

lua_lib_c_args = [
    '-xc++',
    '-D_REENTRANT',
    '-Wshadow',
]

if build_type == 'debug' or build_type == 'debugoptimized'
    lua_lib_c_args += [
        '-ftrapv',
    ]
endif

if host_machine.system() != 'windows'
    # NOTE: -DLUA_USE_POSIX is technically a change from the old Makefile (LUA_USE_LINUX),
    #       but the change should only affect code that is unused at the moment.
    lua_lib_c_args += ['-DLUA_USE_POSIX']
endif

lua_lib = static_library(
    'lua',
    lua_lib_src,
    c_args : lua_lib_c_args,
    override_options : override_options,
)

lua_dep = declare_dependency(
    link_with : lua_lib,
    include_directories : lua_inc,
)

# sltbench dependency for benchmarking
# TODO: write a meson build file for this and move to subprojects/?

sltbench_inc = include_directories('sltbench/include')

sltbench_lib_src = files(
    'sltbench/src/BenchApi.cpp',
    'sltbench/src/BenchmarksContainer.cpp',
    'sltbench/src/Config.cpp',
    'sltbench/src/DoNotOptimize.cpp',
    'sltbench/src/Env.cpp',
    'sltbench/src/Filters.cpp',
    'sltbench/src/IBenchmark.cpp',
    'sltbench/src/MAResultsContainer.cpp',
    'sltbench/src/MeasureAlgo.cpp',
    'sltbench/src/ProcedureBenchmark.cpp',
    'sltbench/src/ProgramOptions.cpp',
    'sltbench/src/Reporters.cpp',
    'sltbench/src/SingleMeasureAlgo.cpp',
    'sltbench/src/SysInfo.cpp',
    'sltbench/src/Verdict.cpp',
)

sltbench_lib = static_library(
    'sltbench',
    sltbench_lib_src,
    include_directories : sltbench_inc,
    override_options : override_options,
)

sltbench_dep = declare_dependency(
    link_with : sltbench_lib,
    include_directories : sltbench_inc,
)
