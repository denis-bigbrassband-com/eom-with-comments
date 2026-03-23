plugins_c_args = ['-DEOM_LOCALEDIR=' + join_paths(datadir / 'locale')]

subdir('fullscreen')
subdir('reload')
subdir('statusbar-comment')

if exif.found()
  subdir('statusbar-date')
endif
