#include <stdio.h>
#include <rz_types.h>
#include <rz_list.h>
#include <rz_util/rz_str.h>
#include <rz_util/rz_file.h>
#include <rz_util/rz_assert.h>
#include <tree_sitter/api.h>

#include <types_parser.h>

#define TS_START_END(node, start, end) \
	do { \
		start = ts_node_start_byte(node); \
		end = ts_node_end_byte(node); \
	} while (0)

static char *ts_node_sub_string(TSNode node, const char *cstr) {
	ut32 start, end;
	TS_START_END(node, start, end);
	return rz_str_newf("%.*s", end - start, cstr + start);
}

static char *ts_node_sub_parent_string(TSNode parent, TSNode node, const char *cstr) {
	ut32 start, end;
	TS_START_END(node, start, end);
	ut32 parent_start = ts_node_start_byte(parent);
	start -= parent_start;
	end -= parent_start;
	return rz_str_newf("%.*s", end - start, cstr + start);
}

void node_malformed_error(TSNode node, const char *nodetype) {
	rz_return_if_fail(nodetype && !ts_node_is_null(node));
	char *string = ts_node_string(node);
	eprintf("Wrongly formed %s:\n%s\n", nodetype, string);
	free(string);
}

// Types can be
// - struct (struct_specifier)
// - union (union_specifier)
// - enum (enum_specifier) (usually prepended by declaration)
// - typedef (type_definition)
// - atomic type

int parse_struct_node(TSNode structnode, const char *text) {
	rz_return_val_if_fail(!ts_node_is_null(structnode), -1);
	rz_return_val_if_fail(ts_node_is_named(structnode), -1);
	int struct_node_child_count = ts_node_named_child_count(structnode);
	if (struct_node_child_count < 1 || struct_node_child_count > 2) {
		node_malformed_error(structnode, "struct");
		return -1;
	}
	printf("struct: has %d children\n", struct_node_child_count);
	if (struct_node_child_count < 2) {
		// Anonymous or forward declaration struct
		TSNode child = ts_node_child(structnode, 1);
		if (!ts_node_is_null(child) && ts_node_is_named(child)) {
			const char *node_type = ts_node_type(child);
			if (!node_type) {
				node_malformed_error(structnode, "struct");
				return -1;
			}
			// "struct bla;"
			if (!strcmp(node_type, "type_identifier")) {
				// We really skip such declarations since they don't
				// make sense for our goal
			// Anonymous struct, "struct { int a; int b; };"
			} else if (!strcmp(node_type, "field_declaration_list")) {
				// FIXME: Support anonymous structures
				eprintf("Anonymous structs aren't supported yet!\n");
				return -1;
			} else {
				node_malformed_error(structnode, "struct");
				return -1;
			}
		} else {
			node_malformed_error(structnode, "struct");
			return -1;
		}
	}
	TSNode struct_name = ts_node_named_child(structnode, 0);
	TSNode struct_body = ts_node_named_child(structnode, 1);
	int body_child_count = ts_node_named_child_count(struct_body);
	const char *realname = ts_node_sub_string(struct_name, text);
	if (!realname || !body_child_count) {
		node_malformed_error(structnode, "struct");
		return -1;
	}
	printf("struct name: %s\n", realname);
	int i;
	for (i = 0; i < body_child_count; i++) {
		printf("struct: processing %d field...\n", i);
		TSNode child = ts_node_named_child(struct_body, i);
		const char *node_type = ts_node_type(child);
		printf("struct: node type is %s\n", node_type);
	}
	return 0;
}

int parse_type_tree(TSNode typenode, const char *text) {
	rz_return_val_if_fail(!ts_node_is_null(typenode), -1);
	rz_return_val_if_fail(ts_node_is_named(typenode), -1);
	const char *node_type = ts_node_type(typenode);
	printf("Node type is %s\n", node_type);
	return 0;
}

// Types can be
// - struct (struct_specifier)
// - union (union_specifier)
// - enum (enum_specifier) (usually prepended by declaration)
// - typedef (type_definition)
// - atomic type
int filter_type_nodes(TSNode node, const char *text) {
	rz_return_val_if_fail(!ts_node_is_null(node), -1);
	// We skip simple nodes (e.g. conditions and braces)
	if (!ts_node_is_named(node)) {
		return 0;
	}
	const char *node_type = ts_node_type(node);
	int result = -1;
	if (!strcmp(node_type, "struct_specifier")) {
		result = parse_struct_node(node, text);
	} else if (!strcmp(node_type, "union_specifier")) {
		result = parse_type_tree(node, text);
	} else if (!strcmp(node_type, "struct_specifier")) {
		result = parse_type_tree(node, text);
	} else if (!strcmp(node_type, "enum_specifier")) {
		result = parse_type_tree(node, text);
	} else if (!strcmp(node_type, "type_definition")) {
		result = parse_type_tree(node, text);
	}

	return result;
}
