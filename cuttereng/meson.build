cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: true)

cuttereng_incdir = include_directories('src/')
cuttereng_lib = library(
  'cuttereng',
  'src/cuttereng.c',
  'src/log.c',
  'src/filesystem.c',
  'src/environment/linux.c',
  'src/json.c',
  'src/hash.c',
  'src/memory.c',
  dependencies: m_dep
)

cuttereng_dep = declare_dependency(include_directories: cuttereng_incdir, link_with: cuttereng_lib)

test_json = executable('test_json', 'tests/test_runner.c', 'tests/json.c', dependencies: [cuttereng_dep])
test('test_json', test_json)
test_hash_table = executable('test_hash_table', 'tests/test_runner.c', 'tests/hash_table.c', dependencies: [cuttereng_dep])
test('test_hash_table', test_hash_table)
