project(
  'rofi-universal',
  'c',
  version: '0.1.0',
  license: 'MIT',
  default_options: ['warning_level=3', 'c_std=c11'],
)

rofi = dependency('rofi', version: '>=1.7.6')
glib = dependency('glib-2.0', version: '>=2.72')
gio = dependency('gio-2.0')
gio_unix = dependency('gio-unix-2.0')
deps = [rofi, glib, gio, gio_unix, dependency('gmodule-2.0'), dependency('cairo')]

shared_module(
  'universal',
  ['src/universal.c', 'src/plugin.c'],
  dependencies: deps,
  install: true,
  install_dir: get_option('libdir') / 'rofi',
  name_prefix: 'lib',
)

install_data(
  'data/rofi-universal',
  install_dir: get_option('bindir'),
  install_mode: 'rwxr-xr-x',
)

install_data(
  'data/config.example.ini',
  install_dir: get_option('datadir') / 'rofi-universal',
)

test_core = executable(
  'test-core',
  ['tests/test_core.c', 'src/universal.c'],
  dependencies: [glib],
  include_directories: include_directories('src'),
)
test('core', test_core)
