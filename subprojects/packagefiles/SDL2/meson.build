project('sdl2', 'c',
  version : '2.0.14',
  license : 'zlib',
  default_options: [])

varr = meson.project_version().split('.')
version_major = varr[0]
version_minor = varr[1]
version_micro = varr[2]

cc = meson.get_compiler('c')

core_deps = [cc.find_library('m', required : false),
  cc.find_library('dl', required : false),
  cc.find_library('pthread', required : false),
]

if host_machine.system() == 'darwin'
  add_languages('objc')
endif

cc = meson.get_compiler('c')
cdata = configuration_data()
check_headers = [
  ['HAVE_ALLOCA_H', 'alloca.h'],
  ['HAVE_SYS_TYPES_H', 'sys/types.h'],
  ['HAVE_STDIO_H', 'stdio.h'],
  ['HAVE_STDLIB_H', 'stdlib.h'],
  ['HAVE_STDARG_H', 'stdarg.h'],
  ['HAVE_MALLOC_H', 'malloc.h'],
  ['HAVE_MEMORY_H', 'memory.h'],
  ['HAVE_STRING_H', 'string.h'],
  ['HAVE_STRINGS_H', 'strings.h'],
  ['HAVE_INTTYPES_H', 'inttypes.h'],
  ['HAVE_STDINT_H', 'stdint.h'],
  ['HAVE_CTYPE_H', 'ctype.h'],
  ['HAVE_MATH_H', 'math.h'],
  ['HAVE_ICONV_H', 'iconv.h'],
  ['HAVE_SIGNAL_H', 'signal.h'],
  ['HAVE_ALTIVEC_H', 'altivec.h'],
  ['HAVE_PTHREAD_NP_H', 'pthread/np.h'],
  ['HAVE_LIBUDEV_H', 'libudev.h'],
  ['HAVE_DBUS_DBUS_H', 'dbus/dbus.h'],
  ['HAVE_STDARG_H', 'stdarg.h'],
  ['HAVE_STDDEF_H', 'stddef.h'],
]

foreach h : check_headers
  if cc.has_header(h.get(1))
    cdata.set(h.get(0), 1)
  endif
endforeach

check_functions = [
  ['HAVE_MALLOC', 'malloc', '#include<stdlib.h>'],
  ['HAVE_CALLOC', 'calloc', '#include<stdlib.h>'],
  ['HAVE_REALLOC', 'realloc', '#include<stdlib.h>'],
  ['HAVE_FREE', 'free', '#include<stdlib.h>'],
  ['HAVE_ALLOCA', 'alloca', '#include<alloca.h>'],
  ['HAVE_GETENV', 'setenv', '#include<stdlib.h>'],
  ['HAVE_SETENV', 'setenv', '#include<stdlib.h>'],
  ['HAVE_PUTENV', 'putenv', '#include<stdlib.h>'],
  ['HAVE_UNSETENV', 'unsetenv', '#include<stdlib.h>'],
  ['HAVE_QSORT', 'qsort', '#include<stdlib.h>'],
  ['HAVE_ABS', 'abs', '#include<stdlib.h>'],
  ['HAVE_BCOPY', 'bcopy', '#include<strings.h>'],
  ['HAVE_MEMSET', 'memset', '#include<string.h>'],
  ['HAVE_MEMCPY', 'memcpy', '#include<string.h>'],
  ['HAVE_MEMMOVE', 'memmove', '#include<string.h>'],
  ['HAVE_MEMCMP', 'memcmp', '#include<string.h>'],
  ['HAVE_STRLEN', 'strlen', '#include<string.h>'],
  ['HAVE_STRLCPY', 'strlcpy', '#include<string.h>'],
  ['HAVE_STRLCAT', 'strlcat', '#include<string.h>'],
  ['HAVE_STRDUP', 'strdup', '#include<string.h>'],
  ['HAVE__STRREV', '_strrev', '#include<string.h>'],
  ['HAVE__STRUPR', '_strupr', '#include<string.h>'],
  ['HAVE__STRLWR', '_strlwr', '#include<string.h>'],
  ['HAVE_INDEX', 'index', '#include<strings.h>'],
  ['HAVE_RINDEX', 'rindex', '#include<strings.h>'],
  ['HAVE_STRCHR', 'strchr', '#include<string.h>'],
  ['HAVE_STRRCHR', 'strrchr', '#include<string.h>'],
  ['HAVE_STRSTR', 'strstr', '#include<string.h>'],
  ['HAVE_ITOA', 'itoa', '#include<stdlib.h>'],
  ['HAVE__LTOA', '_ltoa', '#include<stlib.h>'],
  ['HAVE__UITOA', '_uitoa', '#include<stdlib.h>'],
  ['HAVE__ULTOA', '_ultoa', '#include<stdlib.h>'],
  ['HAVE_STRTOL', 'strtol', '#include<stdlib.h>'],
  ['HAVE_STRTOUL', 'strtoul', '#include<stdlib.h>'],
  ['HAVE__I64TOA', '_i64toa', '#include<stdlib.h>'],
  ['HAVE__UI64TOA', '_uit64toa', '#include<stdlib.h>'],
  ['HAVE_STRTOLL', 'strtoll', '#include<stdlib.h>'],
  ['HAVE_STRTOULL', 'strtoull', '#include<stdlib.h>'],
  ['HAVE_STRTOD', 'strtod', '#include<stdlib.h>'],
  ['HAVE_ATOI', 'atoi', '#include<stdlib.h>'],
  ['HAVE_ATOF', 'atof', '#include<stdlib.h>'],
  ['HAVE_STRCMP', 'strcmp', '#include<string.h>'],
  ['HAVE_STRNCMP', 'strncmp', '#include<string.h>'],
  ['HAVE__STRICMP', '_stricmp', '#include<string.h>'],
  ['HAVE_STRCASECMP', 'strcasecmp', '#include<strings.h>'],
  ['HAVE__STRNICMP', '_strnicmp', '#include<string.h>'],
  ['HAVE_STRNCASECMP', 'strncasecmp', '#include<strings.h>'],
  ['HAVE_VSSCANF', 'vsscanf', '#include<stdio.h>'],
  ['HAVE_VSNPRINTF', 'vsnprintf', '#include<stdio.h>'],
  ['HAVE_ATAN', 'atan', '#include<math.h>'],
  ['HAVE_ATAN2', 'atan2', '#include<math.h>'],
  ['HAVE_ACOS', 'acos', '#include<math.h>'],
  ['HAVE_ASIN', 'asin', '#include<math.h>'],
  ['HAVE_CEIL', 'ceil', '#include<math.h>'],
  ['HAVE_COPYSIGN', 'copysign', '#include<math.h>'],
  ['HAVE_COS', 'cos', '#include<math.h>'],
  ['HAVE_COSF', 'cosf', '#include<math.h>'],
  ['HAVE_FABS', 'fabs', '#include<math.h>'],
  ['HAVE_FLOOR', 'floor', '#include<math.h>'],
  ['HAVE_LOG', 'log', '#include<math.h>'],
  ['HAVE_POW', 'pow', '#include<math.h>'],
  ['HAVE_SCALBN', 'scalbn', '#include<math.h>'],
  ['HAVE_SIN', 'sin', '#include<math.h>'],
  ['HAVE_SINF', 'sinf', '#include<math.h>'],
  ['HAVE_SQRT', 'sqrt', '#include<math.h>'],
  ['HAVE_FSEEKO', 'fseeko', '#include<stdio.h>'],
  ['HAVE_FSEEKO64', 'fseeko64', '#include<stdio.h>'],
  ['HAVE_SIGACTION', 'sigaction', '#include<signal.h>'],
# check token HAVE_SA_SIGACTION
  ['HAVE_SETJMP', 'setjmp', '#include<setjmp.h>'],
  ['HAVE_NANOSLEEP', 'nanosleep', '#include<time.h>'],
  ['HAVE_SYSCONF', 'sysconf', '#include<unistd.h>'],
  ['HAVE_SYSCTLBYNAME', 'sysctlbyname', '#include<sys/sysctl.h>'],
  ['HAVE_CLOCK_GETTIME', 'clock_gettime', '#include<time.h>'],
  ['HAVE_GETPAGESIZE', 'getpagesize', '#include<unistd.h>'],
  ['HAVE_MPROTECT', 'mprotect', '#include<sys/mman.h>'],
# check token HAVE_ICONV
  ['HAVE_PTHREAD_SETNAME_NP', 'pthread_setname_np', '#include<pthread.h>'],
  ['HAVE_PTHREAD_SET_NAME_NP', 'pthread_set_name_np', '#include<pthread.h>'],
# check token HAVE_SEM_TIMEDWAIT
]

foreach f : check_functions
  if cc.has_function(f.get(1), prefix : f.get(2))
    cdata.set(f.get(0), 1)
  endif
endforeach

cdata.set('HAVE_LIBC', 1)
if cc.compiles('''#include<math.h>
double dummy = M_PI
''', name : 'M_PI')
  cdata.set('HAVE_M_PI', 1)
endif

core_args = []#'-DUSING_GENERATED_CONFIG_H']
core_ldflags = []

cdata.set('SDL_SENSOR_DUMMY', 1)

if get_option('dynapi').disabled() # see packagefiles/SDL2/src/dynapi and include/SDL_config.h.meson
  warning('Dynapi disabled. People over at SDL do not like that.')
  cdata.set('SDL_DYNAMIC_API_DISABLED', 1) # FIXME: this is actually not used at the moment
  add_project_arguments('-DSDL_DYNAMIC_API_DISABLED=1', language : 'c')
endif

if host_machine.system() == 'linux'
  cdata.set('HAVE_GCC_ATOMICS', 1)
  cdata.set('HAVE_GCC_SYNC_LOCK_TEST_AND_SET', 1)
  cdata.set('HAVE_PTHREAD_SPINLOCK', 1)
  cdata.set('SDL_POWER_LINUX', 1)
  cdata.set('SDL_THREAD_PTHREAD', 1)
  if get_option('joystick').enabled()
    cdata.set('SDL_JOYSTICK_LINUX', 1)
  endif
    cdata.set('SDL_INPUT_LINUXEV', 1)
    cdata.set('SDL_INPUT_LINUXKD', 1)
  if get_option('haptic').enabled()
    cdata.set('SDL_HAPTIC_LINUX', 1)
    cdata.set('SDL_HAPTIC_XINPUT', 1)
  else
    cdata.set('SDL_HAPTIC_DISABLED', 1)
  endif
  cdata.set('SDL_TIMER_UNIX', 1)
  cdata.set('SDL_FILESYSTEM_UNIX', 1)
  if get_option('loadso').enabled()
    cdata.set('SDL_LOADSO_DLOPEN', 1)
  endif
  cdata.set('SDL_VIDEO_DRIVER_X11', 1)
  cdata.set('SDL_VIDEO_DRIVER_X11_DYNAMIC_XINPUT2', 1)
  cdata.set('SDL_VIDEO_DRIVER_X11_XINPUT2', 1)
  cdata.set('SDL_VIDEO_DRIVER_X11_XINPUT2_SUPPORTS_MULTITOUCH', 1)
  cdata.set('SDL_VIDEO_DRIVER_X11_SUPPORTS_GENERIC_EVENTS', 1)
  cdata.set('SDL_VIDEO_DRIVER_X11_HAS_XKBKEYCODETOKEYSYM', 1) # TODO: auto-detect
  core_args += ['-pthread']
elif host_machine.system() == 'windows'
  cdata.set('SDL_THREAD_WINDOWS', 1)
  if get_option('joystick').enabled()
    cdata.set('SDL_JOYSTICK_HIDAPI', 1)
    cdata.set('SDL_JOYSTICK_DINPUT', 1)
  else
    add_project_arguments('-DSDL_JOYSTICK_HIDAPI=0', language : 'c')
    add_project_arguments('-DSDL_JOYSTICK_DINPUT=0', language : 'c')
    add_project_arguments('-DSDL_JOYSTICK_RAWINPUT=0', language : 'c')
    add_project_arguments('-DSDL_JOYSTICK_WGI=0', language : 'c')
    add_project_arguments('-DSDL_JOYSTICK_XINPUT=0', language : 'c')
  endif
  if get_option('haptic').enabled()
    cdata.set('SDL_HAPTIC_DINPUT', 1)
  else
    cdata.set('SDL_HAPTIC_DISABLED', 1)
    add_project_arguments('-DSDL_HAPTIC_DISABLED=1', language : 'c')
    add_project_arguments('-DSDL_HAPTIC_DINPUT=0', language : 'c')
    add_project_arguments('-DSDL_HAPTIC_XINPUT=0', language : 'c')
  endif
  cdata.set('SDL_TIMER_WINDOWS', 1)
  cdata.set('SDL_FILESYSTEM_WINDOWS', 1)
  cdata.set('SDL_POWER_WINDOWS', 1)
  if get_option('loadso').enabled()
    cdata.set('SDL_LOADSO_WINDOWS', 1)
  endif
  cdata.set('SDL_VIDEO_DRIVER_WINDOWS', 1)
  cdata.set('SDL_VIDEO_RENDER_D3D', 1)
  cdata.set('SDL_AUDIO_DRIVER_WINMM', 1)
  #cdata.set('SDL_VIDEO_OPENGL_EGL', 0)
  #cdata.set('SDL_VIDEO_OPENGL_ES2', 0)
  #cdata.set('SDL_VIDEO_RENDER_OGL_ES2', 0)
  add_project_arguments('-DSDL_VIDEO_OPENGL_EGL=0', language : 'c')
  add_project_arguments('-DSDL_VIDEO_OPENGL_ES2=0', language : 'c')
  add_project_arguments('-DSDL_VIDEO_RENDER_OGL_ES2=0', language : 'c')
#  core_args += '
  core_deps += [cc.find_library('winmm'), cc.find_library('version'), cc.find_library('imm32')]
  if cc.sizeof('void*') == 4
    # Seems to be only needed on 32 bit platforms. Don't know why.
    core_args += ['-DHAVE_LIBC']
  endif
elif host_machine.system() == 'darwin'
  cdata.set('HAVE_GCC_ATOMICS', 1)
  cdata.set('HAVE_GCC_SYNC_LOCK_TEST_AND_SET', 1)
  cdata.set('HAVE_PTHREAD_SPINLOCK', 1)
  cdata.set('SDL_POWER_MACOSX', 1)
  cdata.set('SDL_THREAD_PTHREAD', 1)
  if get_option('joystick').enabled()
    cdata.set('SDL_JOYSTICK_IOKIT', 1)
  endif
  if get_option('haptic').enabled()
    cdata.set('SDL_HAPTIC_IOKIT', 1)
  else
    cdata.set('SDL_HAPTIC_DISABLED', 1)
  endif
  cdata.set('SDL_TIMER_UNIX', 1)
  cdata.set('SDL_FILESYSTEM_UNIX', 1)
  if get_option('loadso').enabled()
  cdata.set('SDL_LOADSO_DLOPEN', 1)
  endif
  cdata.set('SDL_VIDEO_DRIVER_COCOA', 1)
  cdata.set('SDL_AUDIO_DRIVER_COREAUDIO', 1)
  cdata.set('SDL_VIDEO_OPENGL_CGL', 1)
  core_args += ['-pthread']
  core_deps += [dependency('appleframeworks',
    modules : ['CoreFoundation', 'Foundation',
     'QuartzCore', 'CoreAudio',
    'AudioUnit', 'AudioToolbox', 'ForceFeedback',
    'IOKit', 'Cocoa', 'Carbon', 'CoreGraphics'])]
else
  error('Unsupported platform.')
endif

core_inc = [include_directories('include')]
all_sources = []
main_sources = []

alsa_dep = dependency('alsa', required : get_option('alsa'))
x11_dep = dependency('x11', required : false)
xi_dep = dependency('xi', required : false)
xext_dep = dependency('xext', required : false)
gl_dep = dependency('gl', required : false)
xrandr_dep = dependency('xrandr', required : false)
xinerama_dep = dependency('xinerama', required : false)

if gl_dep.found()
  cdata.set('SDL_VIDEO_RENDER_OGL', 1)
  cdata.set('SDL_VIDEO_OPENGL', 1)
endif

if x11_dep.found()
  if gl_dep.found()
    cdata.set('SDL_VIDEO_OPENGL_GLX', 1)
  endif
  if xrandr_dep.found()
    cdata.set('SDL_VIDEO_DRIVER_X11_DYNAMIC_XRANDR', 1)
#    cdata.set('SDL_VIDEO_DRIVER_X11_XRANDR', 1)
  endif
  if xinerama_dep.found()
    cdata.set('SDL_VIDEO_DRIVER_X11_XINERAMA', 1)
#    cdata.set('SDL_VIDEO_DRIVER_X11_DYNAMIC_XINERAMA', 1)
  endif
endif

if alsa_dep.found()
  cdata.set('SDL_AUDIO_DRIVER_ALSA', 1)
#  cdata.set('SDL_AUDIO_DRIVER_ALSA_DYNAMIC', 1)
endif

subdir('include')
subdir('src')

core_deps += [
  x11_dep, xi_dep, xrandr_dep, xext_dep, gl_dep, xinerama_dep,
  alsa_dep,
]

sdl2 = library('sdl2',
  all_sources,
  include_directories : core_inc,
  c_args : core_args,
  link_args : core_ldflags,
  dependencies : core_deps,
)

sdl2main = static_library('sdl2main',
  main_sources,
  include_directories : core_inc,
  c_args : core_args,
)

if host_machine.system() == 'windows'
  sdl2_dep = declare_dependency(link_with : [sdl2, sdl2main],
    include_directories : core_inc)
else
  sdl2_dep = declare_dependency(link_with : sdl2,
    include_directories : core_inc)
endif

# disabled for now
# subdir('test')
