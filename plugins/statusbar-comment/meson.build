desktopfile = configure_file(input: 'statusbar-comment.plugin.desktop.in.in', configuration: pkg_info, output: 'statusbar-comment.plugin.desktop.in')
if get_option('nls')
  i18n.merge_file(
    input: desktopfile,
    output: 'statusbar-comment.plugin',
    install: true,
    install_dir: pluginsdir,
    type: 'desktop',
    po_dir: po_dir,
  )
else
  install_data(desktopfile, install_dir: pluginsdir, rename: ['statusbar-comment.plugin'])
endif

shared_library(
  'statusbar-comment',
  sources: [
    'eom-statusbar-comment-plugin.h',
    'eom-statusbar-comment-plugin.c',
  ],
  c_args: plugins_c_args,
  dependencies: all_deps,
  include_directories: [top_inc, src_inc],
  link_with: [eom_links],
  install: true,
  install_dir: pluginsdir
)
