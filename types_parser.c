#include <stdio.h>
#include <rz_types.h>
#include <rz_list.h>
#include <rz_util/rz_str.h>
#include <rz_util/rz_file.h>
#include <rz_util/rz_assert.h>
#include <rz_type.h>
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

CParserState *c_parser_state_new() {
	CParserState *state = RZ_NEW0(CParserState);
	return state;
}

void c_parser_state_free(CParserState *state) {
	free(state);
	return;
}

// Types can be
// - struct (struct_specifier)
// - union (union_specifier)
// - enum (enum_specifier) (usually prepended by declaration)
// - typedef (type_definition)
// - atomic type

// Identifiers can be simple or arrays or pointers or both

int parse_struct_node(CParserState *state, TSNode structnode, const char *text) {
	rz_return_val_if_fail(!ts_node_is_null(structnode), -1);
	rz_return_val_if_fail(ts_node_is_named(structnode), -1);
	int struct_node_child_count = ts_node_named_child_count(structnode);
	if (struct_node_child_count < 1 || struct_node_child_count > 2) {
		node_malformed_error(structnode, "struct");
		return -1;
	}
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
		eprintf("ERROR: Struct name should not be NULL!\n");
		node_malformed_error(structnode, "struct");
		return -1;
	}
	printf("struct name: %s\n", realname);
	int i;
	for (i = 0; i < body_child_count; i++) {
		if (state->verbose) {
			printf("struct: processing %d field...\n", i);
		}
		TSNode child = ts_node_named_child(struct_body, i);
		const char *node_type = ts_node_type(child);
		// Every field should have (field_declaration) AST clause
		if (strcmp(node_type, "field_declaration")) {
			eprintf("ERROR: Struct field AST should contain (field_declaration) node!\n");
			node_malformed_error(child, "struct field");
			return -1;
		}
		// Every field node should have at least 2 children!
		int field_child_count = ts_node_named_child_count(child);
		if (field_child_count < 2 || field_child_count > 3) {
			eprintf("ERROR: Struct field AST cannot contain less than 2 or more than 3 items");
			node_malformed_error(child, "struct field");
			return -1;
		}
		// Every field can be:
		// - atomic: "int a;" or "char b[20]"
		// - bitfield: int a:7;"
		// - nested: "struct { ... } a;" or "union { ... } a;"
		if (state->verbose) {
			const char *fieldtext = ts_node_sub_string(child, text);
			char *nodeast = ts_node_string(child);
			if (fieldtext && nodeast) {
				printf("field text: %s\n", fieldtext);
				printf("field ast: %s\n", nodeast);
			}
			free(nodeast);
		}
		// 1st case, bitfield
		// AST looks like
		// type: (primitive_type) declarator: (field_identifier) (bitfield_clause (number_literal))
		// Thus it has exactly 3 children
		if (field_child_count == 3) {
			TSNode field_type = ts_node_named_child(child, 0);
			TSNode field_identifier = ts_node_named_child(child, 1);
			TSNode field_bitfield = ts_node_named_child(child, 2);
			if (ts_node_is_null(field_type)
					|| ts_node_is_null(field_identifier)
					|| ts_node_is_null(field_bitfield)) {
				eprintf("ERROR: Struct bitfield type should not be NULL!\n");
				node_malformed_error(child, "struct field");
				return -1;
			}
			// As per C standard bitfields are defined only for atomic types, particularly "int"
			if (strcmp(ts_node_type(field_type), "primitive_type")) {
				eprintf("ERROR: Struct bitfield cannot contain non-primitive bitfield!\n");
				node_malformed_error(child, "struct field");
				return -1;
			}
			const char *real_type = ts_node_sub_string(field_type, text);
			if (!real_type) {
				eprintf("ERROR: Struct bitfield type should not be NULL!\n");
				node_malformed_error(child, "struct field");
				return -1;
			}
			const char *real_identifier = ts_node_sub_string(field_identifier, text);
			if (!real_identifier) {
				eprintf("ERROR: Struct bitfield identifier should not be NULL!\n");
				node_malformed_error(child, "struct field");
				return -1;
			}
			if (ts_node_named_child_count(field_bitfield) != 1) {
				node_malformed_error(child, "struct field");
				return -1;
			}
			TSNode field_bits = ts_node_named_child(field_bitfield, 0);
			if (ts_node_is_null(field_bits)) {
				eprintf("ERROR: Struct bitfield bits AST node should not be NULL!\n");
				node_malformed_error(child, "struct field");
				return -1;
			}
			const char *bits_str = ts_node_sub_string(field_bits, text);
			int bits = atoi(bits_str);
			eprintf("field type: %s field_identifier: %s bits: %d\n", real_type, real_identifier, bits);
		} else {
			printf("field children: %d\n", field_child_count);
			TSNode field_type = ts_node_named_child(child, 0);
			TSNode field_identifier = ts_node_named_child(child, 1);
			if (ts_node_is_null(field_type) || ts_node_is_null(field_identifier)) {
				eprintf("ERROR: Struct field type and identifier should not be NULL!\n");
				node_malformed_error(child, "struct field");
				return -1;
			}
			if (!strcmp(ts_node_type(field_type), "primitive_type")) {
				// 2nd case, atomic field
				// AST looks like
				// type: (primitive_type) declarator: (field_identifier)
				const char *real_type = ts_node_sub_string(field_type, text);
				if (!real_type) {
					eprintf("ERROR: Struct field type should not be NULL!\n");
					node_malformed_error(child, "struct field");
					return -1;
				}
				const char *real_identifier = ts_node_sub_string(field_identifier, text);
				if (!real_identifier) {
					eprintf("ERROR: Struct bitfield identifier should not be NULL!\n");
					node_malformed_error(child, "struct field");
					return -1;
				}
				eprintf("field type: %s field_identifier: %s\n", real_type, real_identifier);
			} else {
				// 3rd case, complex type
				// AST looks like
				// type: (struct_specifier ...) declarator: (field_identifier)
			}
		}
	}
	return 0;
}

// Union is exact copy of struct but size computation is different

int parse_type_tree(CParserState *state, TSNode typenode, const char *text) {
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
int filter_type_nodes(CParserState *state, TSNode node, const char *text) {
	rz_return_val_if_fail(!ts_node_is_null(node), -1);
	// We skip simple nodes (e.g. conditions and braces)
	if (!ts_node_is_named(node)) {
		return 0;
	}
	const char *node_type = ts_node_type(node);
	int result = -1;
	if (!strcmp(node_type, "struct_specifier")) {
		result = parse_struct_node(state, node, text);
	} else if (!strcmp(node_type, "union_specifier")) {
		result = parse_type_tree(state, node, text);
	} else if (!strcmp(node_type, "struct_specifier")) {
		result = parse_type_tree(state, node, text);
	} else if (!strcmp(node_type, "enum_specifier")) {
		result = parse_type_tree(state, node, text);
	} else if (!strcmp(node_type, "type_definition")) {
		result = parse_type_tree(state, node, text);
	}

	return result;
}
