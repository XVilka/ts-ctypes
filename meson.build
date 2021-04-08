project('ts-c-cpp-parser', 'c')

rz_util_lib = dependency('rz_util')

py3_exe = import('python').find_installation()
check_meson_subproject_py = files('sys/check_meson_subproject.py')

repo = '.'
if meson.is_subproject()
  repo = meson.current_source_dir()
  if host_machine.system() == 'windows'
    py_cmd = 'print(__import__("os").readlink(r"@0@"))'.format(repo)
    py_cmd = run_command(py3_exe, '-c', py_cmd)
    if py_cmd.returncode() == 0
      repo = py_cmd.stdout().strip()
      message('ts-c-cpp-parser real path: ' + repo)
    endif
  endif
endif

is_static_build = get_option('static_runtime')
if is_static_build and get_option('default_library') != 'static'
  error('Cannot use `static_runtime` when libraries are dynamically built. Set `--default-library=static` if you want to build statically.')
endif

r = run_command(py3_exe, check_meson_subproject_py, 'tree-sitter')
if r.returncode() == 1 and get_option('subprojects_check')
  error('Subprojects are not updated. Please run `git clean -dxff subprojects/` to delete all local subprojects directories. If you want to compile against current subprojects then set option `subprojects_check=false`.')
endif

tree_sitter_dep = dependency('tree-sitter', required: get_option('use_sys_tree_sitter'), static: is_static_build, fallback: [])
if not tree_sitter_dep.found()
  tree_sitter_proj = subproject('tree-sitter', default_options: ['default_library=static'])
  tree_sitter_dep = tree_sitter_proj.get_variable('tree_sitter_dep')
endif

r = run_command(py3_exe, check_meson_subproject_py, 'tree-sitter-c')
if r.returncode() == 1 and get_option('subprojects_check')
  error('Subprojects are not updated. Please run `git clean -dxff subprojects/` to delete all local subprojects directories. If you want to compile against current subprojects then set option `subprojects_check=false`.')
endif

tree_sitter_c_proj = subproject('tree-sitter-c', default_options: ['default_library=static'])
tree_sitter_c_dep = tree_sitter_c_proj.get_variable('tree_sitter_c_dep')

# Disable for now since it requires C
# see https://github.com/tree-sitter/tree-sitter-cpp/issues/109
#r = run_command(py3_exe, check_meson_subproject_py, 'tree-sitter-cpp')
#if r.returncode() == 1 and get_option('subprojects_check')
#  error('Subprojects are not updated. Please run `git clean -dxff subprojects/` to delete all local subprojects directories. If you want to compile against current subprojects then set option `subprojects_check=false`.')
#endif
#
#tree_sitter_cpp_proj = subproject('tree-sitter-cpp', default_options: ['default_library=static'])
#tree_sitter_cpp_dep = tree_sitter_cpp_proj.get_variable('tree_sitter_cpp_dep')

deps = [
	rz_util_lib,
	tree_sitter_dep,
	tree_sitter_c_dep,
#	tree_sitter_cpp_dep
]

files = [
  'c_cpp_parser.c',
  'types_parser.c',
]

summary({
  'System tree-sitter library': tree_sitter_dep.found() and tree_sitter_dep.type_name() != 'internal'
}, section: 'Configuration', bool_yn: true)

executable('ts-c-cpp-parser', files, dependencies : deps)
