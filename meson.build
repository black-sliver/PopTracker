project('PopTracker', ['c','cpp'],
    meson_version : '>=0.55.0',
    default_options : [
        'cpp_std=c++17',
        'buildtype=debugoptimized',
        'buildtype=debug',
#        'b_lto=true',
        'b_asneeded=true',
        'b_pie=true',
        'b_pic=true',
    ]
)

# TODO: provide custom build file for lua, that does not build lua and luac exe
#       + works on any/latest git
# TODO: provide custom build file for sdl, that disables features we don't need
#       + works on any/latest git
# NOTE: we use a freetype fork (not wrapdb) that allows for easy static building

lib_defaults = [ 'default_library=static' ]

# garbage collect unused stuff
cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')
if cc.has_link_argument('-Wl,--gc-sections') or cc.has_link_argument('-Wl,-dead_strip')
  foreach a: ['-Wl,--gc-sections','-Wl,-dead_strip']
    if cc.has_link_argument(a)
      add_project_arguments(a, language : ['c', 'cpp'])
    endif
  endforeach
  foreach a: ['-ffunction-sections', '-fdata-sections']
    if cc.has_argument(a)
      add_project_arguments(a, language : ['c', 'cpp'])
    endif
  endforeach
endif
if cc.has_argument('-Wno-unused-function')
  add_project_arguments('-Wno-unused-function', language : ['c', 'cpp'])
endif


src = [ # find src/ -print | grep \.cpp$ | sort | sed -e "s/^/    '/g" -e "s/$/',/g"
    'src/main.cpp',
    'src/poptracker.cpp',
    
    'src/core/assets.cpp',
    'src/core/autotracker.cpp',
    'src/core/baseitem.cpp',
    'src/core/imagereference.cpp',
    'src/core/jsonitem.cpp',
    'src/core/layoutnode.cpp',
    'src/core/location.cpp',
    'src/core/luaitem.cpp',
    'src/core/map.cpp',
    'src/core/pack.cpp',
    'src/core/scripthost.cpp',
    'src/core/statemanager.cpp',
    'src/core/tracker.cpp',
    'src/core/zip.cpp',
    
    'src/sd2snes/SD2SNES.cpp',
    
    'src/ui/broadcastwindow.cpp',
    'src/ui/defaulttrackerwindow.cpp',
    'src/ui/item.cpp',
    'src/ui/loadpackwidget.cpp',
    'src/ui/mapwidget.cpp',
    'src/ui/settingswindow.cpp',
    'src/ui/trackerview.cpp',
    'src/ui/trackerwindow.cpp',
    
    'src/uilib/button.cpp',
    'src/uilib/dock.cpp',
    'src/uilib/group.cpp',
    'src/uilib/image.cpp',
    'src/uilib/imagebutton.cpp',
    'src/uilib/imagefilter.cpp',
    'src/uilib/label.cpp',
    'src/uilib/tabs.cpp',
    'src/uilib/ui.cpp',
    'src/uilib/widget.cpp',
    'src/uilib/window.cpp',
]
hdr = [ # as above, but | grep \.h$
]
deps = [] # will be filled below

meson.override_dependency('libtiff-4', disabler())


zlib_proj = subproject('zlib', default_options: lib_defaults)
zlib_dep = zlib_proj.get_variable('zlib_dep')
meson.override_dependency('z', zlib_dep)
meson.override_dependency('libz', zlib_dep)
meson.override_dependency('zlib', zlib_dep)
# deps += zlib_dep

png_proj = subproject('libpng', default_options: lib_defaults)
png_dep = png_proj.get_variable('libpng_dep')
meson.override_dependency('png', png_dep)
meson.override_dependency('libpng', png_dep)
# deps += png_dep

jpeg_proj = subproject('libjpeg', default_options: lib_defaults)
jpeg_dep = jpeg_proj.get_variable('jpeg_dep')
meson.override_dependency('jpeg', jpeg_dep)
meson.override_dependency('libjpeg', jpeg_dep)
# deps += jpeg_dep

ft2_proj = subproject('freetype2', default_options: lib_defaults + [
    'png=enabled',
    'bzip2=disabled',
    'zlib=enabled',
    'harfbuzz=disabled'
])
ft2_dep = ft2_proj.get_variable('freetype_dep')
meson.override_dependency('freetype', ft2_dep)
meson.override_dependency('freetype2', ft2_dep)
# deps += ft2_dep

sdl2_proj = subproject('sdl2', default_options: lib_defaults)
sdl2_dep = sdl2_proj.get_variable('sdl2_dep')
meson.override_dependency('SDL2', sdl2_dep)
deps += sdl2_dep

sdl2_image_proj = subproject('sdl2_image', default_options: lib_defaults)
sdl2_image_dep = sdl2_image_proj.get_variable('sdl2_image_dep')
meson.override_dependency('SDL2_image', sdl2_image_dep)
deps += sdl2_image_dep

sdl2_ttf_proj = subproject('sdl2_ttf', default_options: lib_defaults)
sdl2_ttf_dep = sdl2_ttf_proj.get_variable('sdl2_ttf_dep')
meson.override_dependency('SDL2_ttf', sdl2_ttf_dep)
deps += sdl2_ttf_dep

lua_proj = subproject('lua', default_options: lib_defaults)
lua_dep = lua_proj.get_variable('lua_dep')
meson.override_dependency('lua', lua_dep)
deps += lua_dep

# header-only libs
incdirs = include_directories('lib', 'lib/asio/include', 'lib/miniz')
# and their deps
deps += dependency('threads')
if host_machine.system() == 'windows'
    deps += cpp.find_library('ws2_32', required: true)
endif

executable('poptracker', src,
    dependencies : deps,
    include_directories : incdirs
)
