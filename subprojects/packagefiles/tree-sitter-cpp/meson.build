project('tree-sitter-cpp', version: 'c61212414a3e95b5f7507f98e83de1d638044adc')

ts_cpp_files = [
  'src/parser.c'
]

tree_sitter_proj = subproject('tree-sitter', default_options: ['default_library=static'])
tree_sitter_dep = tree_sitter_proj.get_variable('tree_sitter_dep')

libtscpp = static_library('tree-sitter-cpp', ts_cpp_files,
  include_directories: ['src'],
  implicit_include_directories: false,
  dependencies: tree_sitter_dep.partial_dependency(includes: true)
)

tree_sitter_cpp_dep = declare_dependency(
  link_with: libtscpp,
  include_directories: ['src/tree_sitter'],
  dependencies: tree_sitter_dep
)
