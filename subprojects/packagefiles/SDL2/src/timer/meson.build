all_sources += files(
  'SDL_timer.c',
)

if host_machine.system() == 'windows'
  subdir('windows')
else
  subdir('unix')
endif
#subdir('dummy')
#subdir('haiku')
#subdir('psp')

