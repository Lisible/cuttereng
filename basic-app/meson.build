cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)
basic_app_exe = executable('basic-app', 'main.c', include_directories: cuttereng_incdir, dependencies: [cuttereng_dep, sdl2_dep, m_dep])
