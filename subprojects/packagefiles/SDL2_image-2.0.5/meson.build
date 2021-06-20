project('sdl2-image', 'c')

feature_args = ['-DLOAD_BMP','-DLOAD_GIF']

sdl2_dep = dependency('sdl2')
png_dep = dependency('libpng', required: get_option('png'))
jpeg_dep = dependency('libjpeg', required: get_option('jpeg'))
tif_dep = dependency('libtiff-4', required: get_option('tiff'))

if png_dep.found()
  feature_args += ['-DLOAD_PNG']
endif

if jpeg_dep.found()
  feature_args += ['-DLOAD_JPG']
endif

if tif_dep.found()
  feature_args += ['-DLOAD_TIF']
endif

sdl2_image_lib = library('sdl2image',
  'IMG.c',
  'IMG_bmp.c',
  'IMG_gif.c',
  'IMG_jpg.c',
  'IMG_lbm.c',
  'IMG_pcx.c',
  'IMG_png.c',
  'IMG_pnm.c',
  'IMG_svg.c',
  'IMG_tga.c',
  'IMG_tif.c',
  'IMG_webp.c',
  'IMG_WIC.c',
  'IMG_xcf.c',
  'IMG_xpm.c',
  'IMG_xv.c',
  'IMG_xxx.c',
  c_args: feature_args,
  dependencies: [png_dep, jpeg_dep, tif_dep, sdl2_dep])

sdl2_image_dep = declare_dependency(include_directories: '.',
  link_with: sdl2_image_lib)
