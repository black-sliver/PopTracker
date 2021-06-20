all_sources += files(
  'SDL_thread.c',
)

if host_machine.system() == 'windows'
  subdir('windows')
else
  subdir('pthread')
endif
