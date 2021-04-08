typedef struct {
	bool verbose;

} CParserState;

CParserState *c_parser_state_new();
void c_parser_state_free(CParserState *state);

int filter_type_nodes(CParserState *state, TSNode node, const char *text);
