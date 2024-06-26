cc = meson.get_compiler('c')

cuttereng_deps = []
m_dep = cc.find_library('m', required: true)
cuttereng_deps += m_dep

sdl2_dep = dependency('sdl2')
cuttereng_deps += sdl2_dep

lisiblepng_dep = dependency('lisiblepng', default_options: ['c_args=[\'-DLPNG_DEBUG_LOG\']'])
cuttereng_deps += lisiblepng_dep

lisiblestd_dep = dependency('lisiblestd')
cuttereng_deps += lisiblestd_dep

cc = meson.get_compiler('c')
cuttereng_incdir = include_directories('src/')
cuttereng_lib = library(
  'cuttereng',
  'src/cuttereng.c',
  'src/filesystem.c',
  'src/environment/linux.c',
  'src/json.c',
  'src/engine.c',
  'src/input.c',
  'src/ecs/ecs.c',
  'src/asset.c',
  'src/math/vector.c',
  'src/math/matrix.c',
  'src/math/quaternion.c',
  'src/image.c',
  'src/gltf.c',
  'src/transform.c',
  dependencies: cuttereng_deps,
)

cuttereng_dep = declare_dependency(include_directories: cuttereng_incdir, link_with: [cuttereng_lib], dependencies: [lisiblestd_dep])

test_json = executable('test_json', 'tests/test_runner.c', 'tests/json.c', dependencies: [cuttereng_dep])
test('test_json', test_json)
test_ecs = executable('test_ecs', 'tests/test_runner.c', 'tests/ecs.c', dependencies: [cuttereng_dep])
test('test_ecs', test_ecs)
test_asset = executable('test_asset', 'tests/test_runner.c', 'tests/asset.c', dependencies: [cuttereng_dep])
test('test_asset', test_asset)
test_vector= executable('test_vector', 'tests/test_runner.c', 'tests/math/vector.c', dependencies: [cuttereng_dep])
test('test_vector', test_vector)
test_matrix= executable('test_matrix', 'tests/test_runner.c', 'tests/math/matrix.c', dependencies: [cuttereng_dep])
test('test_matrix', test_matrix)
test_quaternion= executable('test_quaternion', 'tests/test_runner.c', 'tests/math/quaternion.c', dependencies: [cuttereng_dep])
test('test_quaternion', test_quaternion)
test_gltf = executable('test_gltf', 'tests/test_runner.c', 'tests/gltf.c', dependencies: [cuttereng_dep])
test('test_gltf', test_gltf)
