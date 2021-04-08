#include <stdio.h>
#include <rz_types.h>
#include <rz_list.h>
#include <rz_util/rz_file.h>
#include <tree_sitter/api.h>

#include <types_parser.h>

// Declare the `tree_sitter_c` function, which is
// implemented by the `tree-sitter-c` library.
TSLanguage *tree_sitter_c();

// Declare the `tree_sitter_cpp` function, which is
// implemented by the `tree-sitter-cpp` library.
//TSLanguage *tree_sitter_cpp();

int main(int argc, char **argv) {
	if (argc < 1) {
		printf("Usage ts-c-cpp-parser <filename>\n");
		return -1;
	}
	char *file_path = argv[1];
	if (!file_path) {
		printf("Usage ts-c-cpp-parser <filename>\n");
		return -1;
	}
	bool verbose = false;
	if (argc > 2 && argv[2]) {
		// poor-men argument parsing
		if (!strcmp(argv[2], "-v") || !strcmp(argv[2], "--verbose")) {
			verbose = true;
		}
	}

	size_t read_bytes = 0;
	const char *source_code = rz_file_slurp(file_path, &read_bytes);
	if (!source_code || !read_bytes) {
		return -1;
	}
	ut64 file_size = rz_file_size(file_path);
	printf("File size is %"PFMT64d" bytes, read %zu bytes\n", file_size, read_bytes);

	// Create a parser.
	TSParser *parser = ts_parser_new();
	// Set the parser's language (C in this case)
	ts_parser_set_language(parser, tree_sitter_c());

	TSTree *tree = ts_parser_parse_string(
		parser,
		NULL,
		source_code,
		strlen(source_code));

	// Get the root node of the syntax tree.
	TSNode root_node = ts_tree_root_node(tree);
	int root_node_child_count = ts_node_named_child_count(root_node);
	if (!root_node_child_count) {
		printf("Root node is empty!\n");
		ts_tree_delete(tree);
		ts_parser_delete(parser);
		return 0;
	}

	// Some debugging
	if (verbose) {
		printf("root_node (%d children): %s\n", root_node_child_count, ts_node_type(root_node));
		// Print the syntax tree as an S-expression.
		char *string = ts_node_string(root_node);
		printf("Syntax tree: %s\n", string);
		free(string);
	}

	// Create new C parser state
	CParserState *state = c_parser_state_new();
	if (!state) {
		eprintf("CParserState initialization error!\n");
		ts_tree_delete(tree);
		ts_parser_delete(parser);
		return -1;
	}
	state->verbose = verbose;

	// At first step we should handle defines
	// #define
	// #if / #ifdef
	// #else
	// #endif
	// After that, we should process include files and #error/#warning/#pragma
	// And only after that - run the normal C/C++ syntax parsing

	// Filter types function prototypes and start parsing
	int i = 0;
	for (i = 0; i < root_node_child_count; i++) {
		if (verbose) {
			printf("Processing %d child...\n", i);
		}
		TSNode child = ts_node_named_child(root_node, i);
		filter_type_nodes(state, child, source_code);
	}

	c_parser_state_free(state);
	ts_tree_delete(tree);
	ts_parser_delete(parser);
	return 0;
}
