all_sources += files(
  'SDL_mixer.c',
  'SDL_audiodev.c',
  'SDL_audio.c',
  'SDL_audiotypecvt.c',
  'SDL_audiocvt.c',
  'SDL_wave.c',
)

subdir('alsa')
#subdir('android')
#subdir('arts')
#subdir('bsd')
if host_machine.system() == 'darwin'
  subdir('coreaudio')
endif
subdir('directsound')
subdir('disk')
#subdir('dsp')
subdir('dummy')
#subdir('esd')
#subdir('fusionsound')
#subdir('haiku')
#subdir('nas')
#subdir('paudio')
#subdir('psp')
#subdir('pulseaudio')
#subdir('qsa')
#subdir('sndio')
#subdir('sun')
subdir('wasapi')
subdir('winmm')
#subdir('xaudio2')
