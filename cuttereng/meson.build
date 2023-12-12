cc = meson.get_compiler('c')

cuttereng_deps = []
if host_machine.cpu() == 'wasm32'
cc = meson.get_compiler('c')
wgpu_native_dir = get_option('opt_wgpu_native_path')
wgpu_native_lib = cc.find_library('wgpu_native', dirs: [wgpu_native_dir])
wgpu_native_dep = declare_dependency(dependencies: wgpu_native_lib, include_directories: wgpu_native_dir)
cuttereng_deps += wgpu_native_dep
else
m_dep = cc.find_library('m', required: true)
cuttereng_deps += m_dep

sdl2_dep = dependency('sdl2')
cuttereng_deps += sdl2_dep

cc = meson.get_compiler('c')
wgpu_native_dir = get_option('opt_wgpu_native_path')
wgpu_native_lib = cc.find_library('wgpu_native', dirs: [wgpu_native_dir])
wgpu_native_dep = declare_dependency(dependencies: wgpu_native_lib, include_directories: wgpu_native_dir)
cuttereng_deps += wgpu_native_dep
endif
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
  'src/engine.c',
  'src/ecs/ecs.c',
  'src/asset.c',
  'src/renderer/renderer.c',
  'src/renderer/shader.c',
  dependencies: cuttereng_deps,
)

cuttereng_dep = declare_dependency(include_directories: cuttereng_incdir, link_with: cuttereng_lib)

test_json = executable('test_json', 'tests/test_runner.c', 'tests/json.c', dependencies: [cuttereng_dep])
test('test_json', test_json)
test_hash_table = executable('test_hash_table', 'tests/test_runner.c', 'tests/hash_table.c', dependencies: [cuttereng_dep])
test('test_hash_table', test_hash_table)
test_ecs = executable('test_ecs', 'tests/test_runner.c', 'tests/ecs.c', dependencies: [cuttereng_dep])
test('test_ecs', test_ecs)
test_asset = executable('test_asset', 'tests/test_runner.c', 'tests/asset.c', dependencies: [cuttereng_dep])
test('test_asset', test_asset)
test_vector= executable('test_vector', 'tests/test_runner.c', 'tests/math/vector.c', dependencies: [cuttereng_dep])
test('test_vector', test_vector)
test_matrix= executable('test_matrix', 'tests/test_runner.c', 'tests/math/matrix.c', dependencies: [cuttereng_dep])
test('test_matrix', test_matrix)
