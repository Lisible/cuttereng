cuttereng_incdir = include_directories('src/')
cuttereng_lib = library(
  'cuttereng',
  'src/cuttereng.c',
  'src/log.c',
  'src/filesystem.c',
  'src/environment/linux.c'
)

cuttereng_dep = declare_dependency(include_directories: cuttereng_incdir, link_with: cuttereng_lib)
