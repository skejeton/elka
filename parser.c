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

// rules:
// a function will be called with an already allocated node, which is filled
// when a function is called, the lexer already lexed the first token
// when a function returns, the lexer didn't start lexing another expression yet

static
ElkaError next(ElkaParser *p) {
	return elka_lex_next(p->lexer, &p->current_);
}

static
ElkaError peek(ElkaToken *out, ElkaParser *p) {
	return elka_lex_peek(p->lexer, out);
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
	for (int loop=1; loop;) {
		switch (p->current_.type) {
		case ELKA_TT_CARET: // pointer
			elka_node_set(node, elka_node_from(ELKA_NT_POINTER, p->current_));
			node = node->first_child = elka_ast_make(&p->ast, node);
			checkout(next(p));
			break;
		case ELKA_TT_LBRACKET: // array. TODO static arrays
			elka_node_set(node, elka_node_from(ELKA_NT_DYNARR, p->current_));
			node = node->first_child = elka_ast_make(&p->ast, node);
			checkout(expect(p, ELKA_TT_RBRACKET));
			checkout(next(p));
			break;
		case ELKA_TT_IDENT: loop = 0; break;
		default:
			checkout(ELKA_ERR_INVALID_TOKEN);
		}
	}

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
		if (p->current_.type == ELKA_TT_RPAREN) break; // reached the end

		// argument name
		if (p->current_.type != ELKA_TT_IDENT) checkout(ELKA_ERR_INVALID_TOKEN);
		name_stack[name_count++] = p->current_;
	
		checkout(next(p));
		switch (p->current_.type) {
		case ELKA_TT_COMMA:
			continue;
		case ELKA_TT_COLON: // arg group ended
			node = elka_node_set(
				elka_ast_make(&p->ast, root),
				elka_node_from(ELKA_NT_ARG_LIST_PART, p->current_));

			// parse type
			checkout(next(p));
			checkout(par_type(p, elka_ast_make(&p->ast, node)));

			// append types from the group
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

ElkaError par_expr(ElkaParser *p, ElkaNode *node);
ElkaError par_funcall(ElkaParser *p, ElkaNode *node);

// (ident|call|number|string|character|(+|-|!|~)factor|&ident|"("expr")")
ElkaError par_factor(ElkaParser *p, ElkaNode *node) {
	switch (p->current_.type) {
	case ELKA_TT_IDENT: {
		ElkaToken tok;
		checkout(peek(&tok, p));

		if (tok.type == ELKA_TT_LPAREN)
			checkout(par_funcall(p, node));
		else
			elka_node_set(node, elka_node_from(ELKA_NT_VAR, p->current_));

		break;
	} case ELKA_TT_NUMBER:
		elka_node_set(node, elka_node_from(ELKA_NT_NUMBER, p->current_));

		break;
	case ELKA_TT_STRING:
		elka_node_set(node, elka_node_from(ELKA_NT_STRING, p->current_));

		break;
	case ELKA_TT_OP_PLUS:
	case ELKA_TT_OP_MINUS:
	case ELKA_TT_OP_NOT:
	case ELKA_TT_OP_BITNOT:
		elka_node_set(node, elka_node_from(ELKA_NT_OP, p->current_));
		checkout(next(p));
		checkout(par_factor(p, elka_ast_make(&p->ast, node)));

		break;
	case ELKA_TT_LPAREN:
		checkout(next(p));
		checkout(par_expr(p, node));
		checkout(expect(p, ELKA_TT_RPAREN));

		break;
	default:
		checkout(ELKA_ERR_INVALID_TOKEN);
	}

	return ELKA_OK;
}

// factor{(*|/|%|<<|>>|&)term}
ElkaError par_term(ElkaParser *p, ElkaNode *node) {
	ElkaNode term = {};
	checkout(par_factor(p, &term));

	ElkaToken tok;
	checkout(peek(&tok, p));
	if (tok.type == ELKA_TT_ASTERISK || tok.type == ELKA_TT_SLASH  ||
	    tok.type == ELKA_TT_OP_MODULO   || tok.type == ELKA_TT_OP_SHIFTL ||
	    tok.type == ELKA_TT_OP_SHIFTR   || tok.type == ELKA_TT_OP_BITAND ) {
		checkout(next(p));
		elka_node_set(node, elka_node_from(ELKA_NT_OP, p->current_));
		ElkaNode *first_child = elka_ast_make(&p->ast, node);
		*first_child = term;
		checkout(next(p));
		checkout(par_term(p, elka_ast_make(&p->ast, node)));
	} else {
		*node = term;
	}

	return ELKA_OK;
}

// term{(+|-|||~)relation_term}
ElkaError par_relation_term(ElkaParser *p, ElkaNode *node) {
	ElkaNode term = {};
	checkout(par_term(p, &term));

	ElkaToken tok;
	checkout(peek(&tok, p));
	if (tok.type == ELKA_TT_OP_PLUS  || tok.type == ELKA_TT_OP_MINUS     ||
	    tok.type == ELKA_TT_OP_BITOR || tok.type == ELKA_TT_OP_BITNOT    ) {
		checkout(next(p));
		elka_node_set(node, elka_node_from(ELKA_NT_OP, p->current_));
		ElkaNode *first_child = elka_ast_make(&p->ast, node);
		*first_child = term;
		checkout(next(p));
		checkout(par_relation_term(p, elka_ast_make(&p->ast, node)));
	} else {
		*node = term;
	}

	return ELKA_OK;
}

// relationTerm{(==|!=|<|<=|>|>=)relation}
ElkaError par_relation(ElkaParser *p, ElkaNode *node) {
	ElkaNode term = {};
	checkout(par_relation_term(p, &term));

	ElkaToken tok;
	checkout(peek(&tok, p));
	if (tok.type == ELKA_TT_OP_EQUAL   || tok.type == ELKA_TT_OP_NOT_EQUAL    ||
	    tok.type == ELKA_TT_OP_LESSER  || tok.type == ELKA_TT_OP_LESSER_EQUAL ||
	    tok.type == ELKA_TT_OP_GREATER || tok.type == ELKA_TT_OP_GREATER_EQUAL) {
		checkout(next(p));
		elka_node_set(node, elka_node_from(ELKA_NT_OP, p->current_));
		ElkaNode *first_child = elka_ast_make(&p->ast, node);
		*first_child = term;
		checkout(next(p));
		checkout(par_relation(p, elka_ast_make(&p->ast, node)));
	} else {
		*node = term;
	}

	return ELKA_OK;
}

// relation{&&logical_term}
ElkaError par_logical_term(ElkaParser *p, ElkaNode *node) {
	ElkaNode term = {};
	checkout(par_relation(p, &term));

	ElkaToken tok;
	checkout(peek(&tok, p));
	if (tok.type == ELKA_TT_OP_AND) {
		checkout(next(p));
		elka_node_set(node, elka_node_from(ELKA_NT_OP, p->current_));
		ElkaNode *first_child = elka_ast_make(&p->ast, node);
		*first_child = term;
		checkout(next(p));
		checkout(par_logical_term(p, elka_ast_make(&p->ast, node)));
	} else {
		*node = term;
	}

	return ELKA_OK;
}

// logical_term{||expr}
ElkaError par_expr(ElkaParser *p, ElkaNode *node) {
	ElkaNode term = {};
	checkout(par_logical_term(p, &term));
	
	ElkaToken tok;
	checkout(peek(&tok, p));
	if (tok.type == ELKA_TT_OP_OR) {
		checkout(next(p));
		elka_node_set(node, elka_node_from(ELKA_NT_OP, p->current_));
		ElkaNode *first_child = elka_ast_make(&p->ast, node);
		*first_child = term;
		checkout(next(p));
		checkout(par_expr(p, elka_ast_make(&p->ast, node)));
	} else {
		*node = term;
	}

	return ELKA_OK;
}

// ident:=expr
ElkaError par_short_var_decl(ElkaParser *p, ElkaNode *node) {
	elka_node_set(node, elka_node_from(ELKA_NT_VAR_DECL, p->current_));

	checkout(expect(p, ELKA_TT_DEFN));
	checkout(next(p));
	checkout(par_expr(p, elka_ast_make(&p->ast, node)));

	return ELKA_OK;
}

ElkaError par_assign(ElkaParser *p, ElkaNode *node) {
	//TODO
	return ELKA_OK;
}

ElkaError par_funcall(ElkaParser *p, ElkaNode *node) {
	elka_node_set(node, elka_node_from(ELKA_NT_FUNCALL, p->current_));
	checkout(expect(p, ELKA_TT_LPAREN));

	checkout(next(p));
	while (p->current_.type != ELKA_TT_RPAREN) {
		checkout(par_expr(p, elka_ast_make(&p->ast, node)));
		checkout(next(p));
		if (p->current_.type == ELKA_TT_COMMA)
			checkout(next(p));
		else if (p->current_.type != ELKA_TT_RPAREN)
			return ELKA_ERR_INVALID_TOKEN;
	}

	return ELKA_OK;
}

ElkaError par_stmt(ElkaParser *p, ElkaNode *node) {
	switch (p->current_.type) {
	case ELKA_TT_IDENT: {
		ElkaToken tok;
		checkout(peek(&tok, p));
		switch (tok.type) {
		case ELKA_TT_DEFN: // short variable declaration
			checkout(par_short_var_decl(p, node)); break;
		case ELKA_TT_ASSIGN: // assignment
			checkout(par_assign(p, node)); break;
		case ELKA_TT_LPAREN: // function call
			checkout(par_funcall(p, node)); break;
		default:
			checkout(ELKA_ERR_INVALID_TOKEN);
		}

		break;
	} default:
		checkout(ELKA_ERR_INVALID_TOKEN);
	}

	return ELKA_OK;
}

// {stmt[stmt]}
ElkaError par_block(ElkaParser *p, ElkaNode *node) {
	ElkaNode *root = node;
	elka_node_set(root, elka_node_from(ELKA_NT_BLOCK, p->current_));

	for (;;) {
		checkout(next(p));
		if (p->current_.type == ELKA_TT_RBRACE) break;

		checkout(par_stmt(p, elka_ast_make(&p->ast, root)));
	}

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
		ElkaToken tok;
		checkout(peek(&tok, p));
		if (tok.type == ELKA_TT_COLON) {
			checkout(next(p)); // skip colon

			checkout(next(p));
			if (p->current_.type == ELKA_TT_LPAREN) // multiple returns
				checkout(par_type_list(p, elka_ast_make(&p->ast, root)));
			else // single return
				checkout(par_type(p, elka_ast_make(&p->ast, root)));
			checkout(peek(&p->current_, p));
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

