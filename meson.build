project('simfan', ['c'])
cc = meson.get_compiler('c')

libconfig = dependency('libconfig')

# add_project_arguments('-march=native', '-mtune=generic', '-O2', '-pipe', '-fno-plt', '-fexceptions', '-Wp,-D_FORTIFY_SOURCE=2', '-Wformat', '-Werror=format-security', '-fstack-clash-protection', '-fcf-protection', language : 'c')

executable(meson.project_name(), ['src/main.c', 'src/config.c', 'src/fan.c', 'src/temp.c'],
include_directories : ['inc'],
dependencies: [libconfig], install: true)
