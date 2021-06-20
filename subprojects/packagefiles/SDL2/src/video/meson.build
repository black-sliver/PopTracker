all_sources += files(
  'SDL_blit.c',
  'SDL_stretch.c',
  'SDL_surface.c',
  'SDL_fillrect.c',
  'SDL_clipboard.c',
  'SDL_blit_slow.c',
  'SDL_RLEaccel.c',
  'SDL_bmp.c',
  'SDL_blit_A.c',
  'SDL_blit_N.c',
  'SDL_blit_1.c',
  'SDL_blit_0.c',
  'SDL_blit_copy.c',
  'SDL_blit_auto.c',
  'SDL_rect.c',
  'SDL_pixels.c',
  'SDL_shape.c',
  'SDL_egl.c',
  'SDL_video.c',
  'SDL_vulkan_utils.c',
  'SDL_yuv.c',
)

if host_machine.system() == 'windows'
  subdir('windows')
elif host_machine.system() == 'darwin'
  subdir('cocoa')
elif x11_dep.found()
  subdir('x11')
else
  error('Unknown platform.')
endif
#subdir('android')
#subdir('cocoa')
#subdir('directfb')
subdir('dummy')
#subdir('haiku')
#subdir('mir')
#subdir('pandora')
#subdir('psp')
#subdir('raspberry')
#subdir('uikit')
#subdir('wayland')
#subdir('winrt')
subdir('yuv2rgb')
