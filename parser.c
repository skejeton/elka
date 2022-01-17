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
	checkout(elka_lex_next(p->lexer, &p->current_));
	return ELKA_OK;
}

ElkaError expect(ElkaParser *p, ElkaTokenType t) {
	checkout(next(p));
	if (p->current_.type != t)
		return ELKA_ERR_INVALID_TOKEN;
	return ELKA_OK;
}

static
ElkaError par_top_level(ElkaParser *p, ElkaNode *node) {
	
}

ElkaError elka_parser_parse(ElkaParser *parser) {
    checkout(elka_lex_next(parser->lexer, &parser->current_));
    while(!parser->eof) checkout(par_fn(parser));
    return ELKA_OK;
}


