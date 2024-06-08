cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

basic_app_c_args = []
basic_app_link_args = []

if is_playdate_build
  basic_app_c_args += playdate_bin_c_flags
  basic_app_link_args += playdate_bin_link_args
elif use_playdate_backend
  basic_app_c_args += playdate_sim_bin_c_flags
endif

if use_sdl_backend
basic_app_c_args += '-DCUTTERENG_SDL_BACKEND'
endif
if use_playdate_backend
basic_app_c_args += '-DCUTTERENG_PLAYDATE_BACKEND'
endif

if not is_playdate_build
basic_app_exe = executable('basic-app', 'main.c', dependencies: [cuttereng_dep, m_dep], c_args: basic_app_c_args)

if use_playdate_backend
basic_app_sim_bin = shared_library(
  'pdex', 
  playdate_glue_code_src +  ['main.c'],  
  dependencies: [cuttereng_dep, playdate_sdk_dep, m_dep], 
  c_args: basic_app_c_args,
  name_prefix: '', 
  name_suffix: 'so', 
  native: true
)

sh = find_program('sh')
basic_app_sim_pdx_name = 'basic-app-sim.pdx'
basic_app_sim_pdx = custom_target(
  basic_app_sim_pdx_name,
  output: basic_app_sim_pdx_name,
  command: [sh, [meson.project_source_root() / 'scripts' / 'generate-sim-pdx.sh', meson.project_name() + '-sim', playdate_sdk_path]],
  depends: basic_app_sim_bin,
  build_always_stale: true,
  build_by_default: true
)

playdate_simulator = find_program('PlaydateSimulator')
run_target(
  'run-sim',
  command: [playdate_simulator, 'basic_app' / basic_app_sim_pdx_name],
)
run_target(
  'build-run-sim',
  command: [playdate_simulator, basic_app_sim_pdx_name],
  depends: [basic_app_sim_pdx]
)
endif
else
basic_app_bin_pdx = executable(
  'pdex.elf',  
  playdate_glue_code_src +  ['main.c'],  
  c_args: basic_app_c_args, 
  link_args: basic_app_link_args, 
  dependencies: [playdate_sdk_dep, cuttereng_dep], 
)

sh = find_program('sh')
basic_app_pdx= custom_target(
  'basic-app.pdx',
  output: 'basic-app.pdx',
  command: [sh, [meson.project_source_root() / 'scripts' / 'generate-pdx.sh', 'basic-app']],
  depends: basic_app_bin_pdx,
  build_always_stale: true,
  build_by_default: true
)
endif



