project('cuttereng', 'c', default_options: ['c_std=gnu18', 'warning_level=3'])

if get_option('buildtype') == 'debug'
  add_project_arguments('-DDEBUG', language: 'c')
endif

add_project_arguments('-fstack-protector', language: 'c')
add_project_arguments('-Wno-gnu-zero-variadic-macro-arguments', language: 'c')
add_project_arguments('-Wno-gnu-binary-literal', language: 'c')

subdir('cuttereng')
subdir('basic-app')
