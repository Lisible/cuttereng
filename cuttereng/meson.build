cc = meson.get_compiler('c')

cuttereng_deps = []
lisiblepng_dep = dependency('lisiblepng', default_options: ['c_args=[\'-DLPNG_DEBUG_LOG\']'])
cuttereng_deps += lisiblepng_dep
if use_sdl_backend
m_dep = cc.find_library('m', required: true)
cuttereng_deps += m_dep

sdl2_dep = dependency('sdl2')
cuttereng_deps += sdl2_dep
endif
if use_playdate_backend
cuttereng_deps += playdate_sdk_dep
endif
 

lisiblestd_dep = dependency('lisiblestd')
cuttereng_deps += lisiblestd_dep
cuttereng_incdir = include_directories('src/')

cuttereng_src = [
  'src/cuttereng.c',
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
]

if is_playdate_build
cuttereng_src += 'src/filesystem_pd.c'
else
cuttereng_src += 'src/filesystem.c'
endif

cuttereng_c_args = []
if use_sdl_backend
cuttereng_c_args += '-DCUTTERENG_SDL_BACKEND'
endif
if use_playdate_backend
cuttereng_c_args += '-DCUTTERENG_PLAYDATE_BACKEND'
cuttereng_c_args += '-DTARGET_EXTENSION'
endif

cuttereng_lib = static_library(
  'cuttereng',
  cuttereng_src,
  dependencies: cuttereng_deps,
  c_args: cuttereng_c_args,
)

cuttereng_dep = declare_dependency(include_directories: cuttereng_incdir, link_with: [cuttereng_lib], dependencies: cuttereng_deps)

if not meson.is_cross_build()
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
endif
