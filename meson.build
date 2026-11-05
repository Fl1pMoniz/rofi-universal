project(
  'rofi-universal',
  'c',
  version: '0.8.0',
  license: 'MIT',
  default_options: ['warning_level=3', 'c_std=c11'],
)

rofi = dependency('rofi', version: '>=1.7.6')
glib = dependency('glib-2.0', version: '>=2.72')
gio = dependency('gio-2.0')
gio_unix = dependency('gio-unix-2.0')
json_glib = dependency('json-glib-1.0', version: '>=1.6')
deps = [rofi, glib, gio, gio_unix, dependency('gmodule-2.0'),
        dependency('cairo'), json_glib]

shared_module(
  'universal',
  ['src/universal.c', 'src/query.c', 'src/plugin.c', 'src/process.c',
   'src/weather.c', 'src/fetch.c', 'src/dictionary.c'],
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
  ['tests/test_core.c', 'src/universal.c', 'src/query.c'],
  dependencies: [glib],
  include_directories: include_directories('src'),
)
test('core', test_core)

test_query_regression = executable(
  'test-query-regression',
  ['tests/test_query_regression.c', 'src/universal.c', 'src/query.c'],
  dependencies: [glib],
  include_directories: include_directories('src'),
)
test('query-regression', test_query_regression)

benchmark_core = executable(
  'benchmark-core',
  ['tests/benchmark_core.c', 'src/universal.c', 'src/query.c'],
  dependencies: [glib],
  include_directories: include_directories('src'),
  build_by_default: false,
)

test_process = executable(
  'test-process',
  ['tests/test_process.c', 'src/process.c'],
  dependencies: [glib, gio],
  include_directories: include_directories('src'),
)
test('process', test_process)

test_weather = executable(
  'test-weather',
  ['tests/test_weather.c', 'src/weather.c'],
  dependencies: [glib],
  include_directories: include_directories('src'),
)
test('weather', test_weather)

test_fetch = executable(
  'test-fetch',
  ['tests/test_fetch.c', 'src/fetch.c'],
  dependencies: [glib],
  include_directories: include_directories('src'),
)
test('fetch', test_fetch)

test_dictionary = executable(
  'test-dictionary',
  ['tests/test_dictionary.c', 'src/dictionary.c'],
  dependencies: [glib, json_glib],
  include_directories: include_directories('src'),
)
test('dictionary', test_dictionary)
