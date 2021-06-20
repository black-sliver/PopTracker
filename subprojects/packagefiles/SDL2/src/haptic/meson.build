if get_option('haptic').enabled()
  all_sources += files(
    'SDL_haptic.c',
  )
  if host_machine.system() == 'linux'
    subdir('linux')
  elif host_machine.system() == 'windows'
    subdir('windows')
  elif host_machine.system() == 'darwin'
    subdir('darwin')
  else
    subdir('dummy')
  endif
endif
