project('zathura-markdown', 'c',
  version: '2026.05.10',
  meson_version: '>=1',
  default_options: ['c_std=c23', 'warning_level=3']
)

version = meson.project_version()

cc = meson.get_compiler('c')

datadir = get_option('datadir')
prefix = get_option('prefix')
metainfodir = join_paths(datadir, 'metainfo')
desktopdir = join_paths(datadir, 'applications')

# required dependencies
mupdf_required_version_major = 1
mupdf_required_version_minor = 26

zathura = dependency('zathura', version: '>=2026.07.18')
girara = dependency('girara')
glib = dependency('glib-2.0')
cairo = dependency('cairo')
cmark = dependency('libcmark', version: '>=0.29')
mupdf = dependency('mupdf', required: false, version: '>=@0@.@1@'.format(mupdf_required_version_major, mupdf_required_version_minor))

build_dependencies = [
  zathura,
  girara,
  glib,
  cairo,
  cmark,
]

if not mupdf.found()
  # normal build of mupdf
  mupdf = cc.find_library('mupdf', has_headers: ['mupdf/fitz/version.h', 'mupdf/fitz.h', 'mupdf/pdf.h'], required: true)
  mupdfthird = cc.find_library('mupdf-third')
  version_check = '''
#include <mupdf/fitz/version.h>

#if FZ_VERSION_MAJOR < @0@ || (FZ_VERSION_MAJOR == @0@ && FZ_VERSION_MINOR < @1@)
#error "mupdf @0@.@1@ or newer is requried"
#endif
'''.format(mupdf_required_version_major, mupdf_required_version_minor)
  if not cc.compiles(version_check, dependencies: [mupdf])
    error('mupdf @0@.@1@ or newer is required'.format(mupdf_required_version_major, mupdf_required_version_minor))
  endif

  build_dependencies += [mupdf, mupdfthird]
else
  # build from Debian's libmupdf-dev
  build_dependencies += [mupdf]
endif

if get_option('plugindir') == ''
  plugindir = zathura.get_variable(pkgconfig: 'plugindir')
else
  plugindir = get_option('plugindir')
endif

# defines
defines = [
  '-DVERSION="@0@"'.format(version),
  '-D_DEFAULT_SOURCE',
]

# compile flags
flags = [
  '-Werror=implicit-function-declaration',
  '-Werror=vla',
  '-Werror=int-conversion',
  '-Werror=maybe-uninitialized'
]
flags = cc.get_supported_arguments(flags)

sources = files(
  'zathura-markdown/document.c',
  'zathura-markdown/index.c',
  'zathura-markdown/links.c',
  'zathura-markdown/page.c',
  'zathura-markdown/plugin.c',
  'zathura-markdown/render.c',
  'zathura-markdown/search.c',
  'zathura-markdown/select.c',
  'zathura-markdown/utils.c'
)

markdown = shared_module('markdown',
  sources,
  dependencies: build_dependencies,
  c_args: defines + flags,
  install: true,
  install_dir: plugindir,
  gnu_symbol_visibility: 'hidden'
)

subdir('data')
