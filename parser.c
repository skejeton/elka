#include "ast.h"
#include "parser.h"
#include "codegen.h"
#include "general.h"
#include "lexer.h"
#include "refl.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

#ifndef checkout
#define checkout(x) do { ElkaError err; if((err = (x))) { fprintf(stderr, ">>> %d %-20s %s:%d\n", err, __FUNCTION__, __FILE__, __LINE__); return err;} } while (0)
#endif

static
ElkaError next(ElkaParser *p) {
	return elka_lex_next(p->lexer, &p->current_);
}

static
ElkaError expect(ElkaParser *p, ElkaTokenType t) {
	checkout(next(p));
	if (p->current_.type != t)
		return ELKA_ERR_INVALID_TOKEN;

	return ELKA_OK;
}

static
ElkaError par_rcv_signature(ElkaParser *p, ElkaNode *node) {
	// TODO
	return ELKA_OK;
}

static
ElkaError par_type(ElkaParser *p, ElkaNode *node) {
	// TODO
	elka_node_set(node, elka_node_from(ELKA_NT_TYPE, p->current_));

	return ELKA_OK;
}

/*
 * (------------
 * |            \
 * :--------     :--
 * |   |  | \    |  \
 * int a  b  c   ^   name
 *               |
 *               str
 */
static
ElkaError par_arg_list(ElkaParser *p, ElkaNode *node) {
	elka_node_set(node, elka_node_from(ELKA_NT_ARG_LIST, p->current_));
	const ElkaNode *root = node;

	if (p->current_.type != ELKA_TT_LPAREN) checkout(ELKA_ERR_INVALID_TOKEN);
	ElkaToken name_stack[128];
	int name_count = 0;

	for (int parse=1; parse; ) {
		checkout(expect(p, ELKA_TT_IDENT));
		name_stack[name_count++] = p->current_;
	
		checkout(next(p));
		switch (p->current_.type) {
		case ELKA_TT_COMMA:
			continue;
		case ELKA_TT_COLON:
			node = elka_node_set(
				elka_ast_make(&p->ast, root),
				elka_node_from(ELKA_NT_ARG_LIST_PART, p->current_));

			// parse type
			checkout(next(p));
			checkout(par_type(p, elka_ast_make(&p->ast, node)));

			for (int i=0; i < name_count; ++i) {
				elka_node_set(
					elka_ast_make(&p->ast, node),
					elka_node_from(ELKA_NT_IDENT, name_stack[i]));
			} name_count = 0;

			checkout(next(p));
			switch (p->current_.type) {
			case ELKA_TT_COMMA:
				break;
			case ELKA_TT_RPAREN:
				parse = 0;
				break;
			default:
				checkout(ELKA_ERR_INVALID_TOKEN);
			}
			break;
		default:
			checkout(ELKA_ERR_INVALID_TOKEN);
		}
	}

	return ELKA_OK;
}

ElkaError par_block(ElkaParser *p, ElkaNode *node) {
	// TODO
	return ELKA_OK;
}

static
ElkaError par_fn_decl(ElkaParser *p, ElkaNode *node) {
	elka_node_set(node, elka_node_from(ELKA_NT_FN_PROTOTYPE, p->current_));
	ElkaNode *root = node;

	checkout(next(p));
	switch (p->current_.type) {
	case ELKA_TT_LPAREN: // methods
		checkout(par_rcv_signature(p, elka_ast_make(&p->ast, root)));

	// FALLTHROUGH
	case ELKA_TT_IDENT:
		node = elka_ast_make(&p->ast, root);
		elka_node_set(node, elka_node_from(ELKA_NT_IDENT, p->current_));
		checkout(next(p));

		if (p->current_.type == ELKA_TT_ASTERISK) {
			node = elka_ast_make(&p->ast, root);
			elka_node_set(node, elka_node_from(ELKA_NT_EXPORT_SYMBOL, p->current_));
			checkout(next(p));
		}

		if (p->current_.type != ELKA_TT_LPAREN)
			return ELKA_ERR_INVALID_TOKEN;

		checkout(par_arg_list(p, elka_ast_make(&p->ast, root)));

		checkout(next(p));
		if (p->current_.type != ELKA_TT_LBRACE)
			break;

		root->type = ELKA_NT_FN_DECL;
		checkout(par_block(p, elka_ast_make(&p->ast, root)));

		break;
	default:
		return ELKA_ERR_INVALID_TOKEN;
	}

	return ELKA_OK;
}

static
ElkaError par_top_level(ElkaParser *p, ElkaNode *node) {
	checkout(next(p));

	switch (p->current_.type) {
	case ELKA_TT_KW_FN:
		checkout(par_fn_decl(p, node));
	default:
		return ELKA_ERR_INVALID_TOKEN;
	}

	return ELKA_OK;
}

ElkaError elka_parser_parse(ElkaParser *parser) {
	//checkout(elka_lex_next(parser->lexer, &parser->current_));
	parser->ast = elka_ast_init();

	ElkaError err = ELKA_OK;
	while (!err)
		err = par_top_level(parser, elka_ast_make(&parser->ast, NULL));
	if (err != ELKA_ERR_EOF)
		checkout(err);

	elka_ast_pretty_print(parser->lexer, &parser->ast);
	return ELKA_OK;
}

