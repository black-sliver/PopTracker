# Use the preconfigured header on platforms that provide
# one. Currently only Windows, add others as needed.
if host_machine.system() != 'windows'
  # The config file is at root because we want to
  # ensure that it comes before the include subdir
  # in search order and include also has a SDL_config.h.
  configure_file(input : 'SDL_config.h.meson',
    output : 'SDL_config.h',
    configuration : cdata)
    core_args = ['-DUSING_GENERATED_CONFIG_H']
endif
