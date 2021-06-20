#all_sources += files(
#  'SDL_hidapi.c',
#)

if host_machine.system() == 'linux'
  subdir('linux')
elif host_machine.system() == 'windows'
  subdir('windows')
elif host_machine.system() == 'darwin'
  subdir('mac')
else
  error('Unsupported platform.')
endif

core_inc += include_directories('hidapi')