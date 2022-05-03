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
ElkaError peek(ElkaParser *p) {
	return elka_lex_peek(p->lexer, &p->current_);
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

// typeMod <- (^|[])[typeMod]
// type <- [typeMod][ident"."]ident
static
ElkaError par_type(ElkaParser *p, ElkaNode *node) {
	// TODO
	elka_node_set(node, elka_node_from(ELKA_NT_TYPE, p->current_));

	return ELKA_OK;
}

static
ElkaError par_type_list(ElkaParser *p, ElkaNode *node) {
	ElkaNode *root = node;
	elka_node_set(root, elka_node_from(ELKA_NT_TYPE_LIST, p->current_));

	for (;;) {
		checkout(next(p));
		checkout(par_type(p, elka_ast_make(&p->ast, root)));
		checkout(next(p));
		if (p->current_.type == ELKA_TT_COMMA) continue;
		if (p->current_.type == ELKA_TT_RPAREN) break;
	}

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
	ElkaNode *root = node;

	if (p->current_.type != ELKA_TT_LPAREN) checkout(ELKA_ERR_INVALID_TOKEN);
	ElkaToken name_stack[128];
	int name_count = 0;

	for (int parse=1; parse; ) {
		checkout(next(p));
		if (p->current_.type == ELKA_TT_RPAREN) break;
		if (p->current_.type != ELKA_TT_IDENT) checkout(ELKA_ERR_INVALID_TOKEN);
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

// fundef = "fn" [ rcv_signature ] ident["*"]arg_list[ ":" (type|type_list) ] [ block ]
static
ElkaError par_fn_decl(ElkaParser *p, ElkaNode *node) {
	elka_node_set(node, elka_node_from(ELKA_NT_FN_PROTOTYPE, p->current_));
	ElkaNode *root = node;

	checkout(next(p));
	switch (p->current_.type) {
	case ELKA_TT_LPAREN: // methods
		checkout(par_rcv_signature(p, elka_ast_make(&p->ast, root)));

	/**FALLTHROUGH**/
	case ELKA_TT_IDENT:
		// name
		node = elka_ast_make(&p->ast, root);
		elka_node_set(node, elka_node_from(ELKA_NT_IDENT, p->current_));
		checkout(next(p));

		// export symbol
		if (p->current_.type == ELKA_TT_ASTERISK) {
			node = elka_ast_make(&p->ast, root);
			elka_node_set(node, elka_node_from(ELKA_NT_EXPORT_SYMBOL, p->current_));
			checkout(next(p));
		}

		// arguments
		if (p->current_.type != ELKA_TT_LPAREN)
			return ELKA_ERR_INVALID_TOKEN;

		checkout(par_arg_list(p, elka_ast_make(&p->ast, root)));

		// return values
		checkout(peek(p));
		if (p->current_.type == ELKA_TT_COLON) {
			checkout(next(p)); // skip colon

			checkout(next(p));
			if (p->current_.type == ELKA_TT_LPAREN) // multiple returns
				checkout(par_type_list(p, elka_ast_make(&p->ast, root)));
			else // single return
				checkout(par_type(p, elka_ast_make(&p->ast, root)));
			checkout(peek(p));
		}

		if (p->current_.type != ELKA_TT_LBRACE) // prototype
			break;

		checkout(next(p));
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
	switch (p->current_.type) {
	case ELKA_TT_KW_FN:
		checkout(par_fn_decl(p, node));
		break;
	default:
		return ELKA_ERR_INVALID_TOKEN;
	}

	return ELKA_OK;
}

ElkaError elka_parser_parse(ElkaParser *parser) {
	parser->ast = elka_ast_init();
	ElkaNode *root = elka_ast_make(&parser->ast, NULL);

	checkout(next(parser));
	for (; parser->current_.type != ELKA_TT_EOF; ) {
		checkout(par_top_level(parser, elka_ast_make(&parser->ast, root)));
		checkout(next(parser));
	}

	elka_ast_pretty_print(parser->lexer, &parser->ast);
	return ELKA_OK;
}

