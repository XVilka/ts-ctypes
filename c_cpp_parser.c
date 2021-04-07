#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <tree_sitter/api.h>

// Declare the `tree_sitter_c` function, which is
// implemented by the `tree-sitter-c` library.
TSLanguage *tree_sitter_c();

// Declare the `tree_sitter_cpp` function, which is
// implemented by the `tree-sitter-cpp` library.
//TSLanguage *tree_sitter_cpp();


int main() {
  // Create a parser.
  TSParser *parser = ts_parser_new();

  // Set the parser's language (C in this case)
  ts_parser_set_language(parser, tree_sitter_c());

  // Build a syntax tree based on source code stored in a string.
  const char *source_code = "typedef struct bla { int a; char **b[52]; } bla_t;";

  TSTree *tree = ts_parser_parse_string(
    parser,
    NULL,
    source_code,
    strlen(source_code)
  );

  // Get the root node of the syntax tree.
  TSNode root_node = ts_tree_root_node(tree);

  // Get some child nodes.
  TSNode typedef_node = ts_node_named_child(root_node, 0);
  TSNode struct_node = ts_node_named_child(typedef_node, 0);

  // Check that the nodes have the expected types.

  printf("root_node: %s\n", ts_node_type(root_node));
  printf("root_node: %s\n", ts_node_type(typedef_node));
  printf("root_node: %s\n", ts_node_type(struct_node));

  // Check that the nodes have the expected child counts.
  /*
  assert(ts_node_child_count(root_node) == 1);
  assert(ts_node_child_count(array_node) == 5);
  assert(ts_node_named_child_count(array_node) == 2);
  assert(ts_node_child_count(number_node) == 0);
  */

  // Print the syntax tree as an S-expression.
  char *string = ts_node_string(root_node);
  printf("Syntax tree: %s\n", string);

  // Free all of the heap-allocated memory.
  free(string);
  ts_tree_delete(tree);
  ts_parser_delete(parser);
  return 0;
}

