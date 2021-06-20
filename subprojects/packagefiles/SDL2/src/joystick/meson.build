all_sources += files(
  'SDL_joystick.c',
  'SDL_gamecontroller.c',
)

subdir('hidapi')
subdir('steam')

if host_machine.system() == 'linux'
  subdir('linux')
elif host_machine.system() == 'windows'
  subdir('windows')
elif host_machine.system() == 'darwin'
  subdir('darwin')
else
  error('Unsupported platform.')
endif
