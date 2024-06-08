project('cuttereng', 'c', default_options: ['c_std=gnu18', 'warning_level=3'])


use_sdl_backend = get_option('use_sdl_backend')
use_playdate_backend = get_option('use_playdate_backend')
is_playdate_build = meson.get_external_property('is_playdate_build', false)
if is_playdate_build
use_sdl_backend = false
use_playdate_backend = true
endif

if use_playdate_backend
playdate_sdk_path = get_option('playdate_sdk_path')
playdate_sdk_incdir = playdate_sdk_path / 'C_API'
playdate_sdk_dep = declare_dependency(include_directories: playdate_sdk_incdir)
playdate_glue_code_src = [playdate_sdk_path / 'C_API' / 'buildsupport' / 'setup.c']
playdate_bin_c_flags = [
  '-DTARGET_PLAYDATE=1',
  '-DTARGET_EXTENSION=1',
  '-mthumb',
  '-mcpu=cortex-m7',
  '-mfloat-abi=hard',
  '-mfpu=fpv5-sp-d16',
  '-D__FPU_USED=1',
  '-falign-functions=16',
  '-fomit-frame-pointer',
  '-gdwarf-2',
  '-fverbose-asm',
  '-ffunction-sections',
  '-fdata-sections',
  '-mword-relocations',
  '-fno-common',
  '-fno-exceptions',
  '-O2',
]
playdate_sim_bin_c_flags = [
  '-DTARGET_SIMULATOR=1',
  '-DTARGET_EXTENSION=1',
]
playdate_bin_link_args = [
  '-nostartfiles',
  '-mthumb', 
  '-mcpu=cortex-m7', 
  '-mfloat-abi=hard', 
  '-mfpu=fpv5-sp-d16',
  '-D__FPU_USED=1',
  '-T@0@'.format(playdate_sdk_path / 'C_API' / 'buildsupport' / 'link_map.ld'),
  '-Wl,-Map=game.map,--cref,--gc-sections,--no-warn-mismatch,--emit-relocs',
  '--specs=nosys.specs'
]
endif

if get_option('buildtype') == 'debug'
  add_project_arguments('-DDEBUG', language: 'c')
endif

add_project_arguments('-fstack-protector', language: 'c')

subdir('cuttereng')
subdir('basic-app')
