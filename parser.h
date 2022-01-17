#ifndef ELKA_PARSER_H__
#define ELKA_PARSER_H__
#include "lexer.h"
#include "codegen.h"
#include <stdbool.h>

typedef struct ElkaParserError {
    // FIXME: For now we're just gonna store text, yeah
    char txt[1024];
} ElkaParserError;

typedef struct ElkaParser {
    bool eof;
    size_t stack_base;
    ElkaLexer *lexer;
    ElkaCodegen cg;
    ElkaToken current_;
    char tmpstrbuf_[1024];
    ElkaRefl *refl;
    
    // I should probably move this to reflection
    ElkaReflItem last_item;
    ElkaReflItem *return_type;
    
    
    ElkaParserError err;
} ElkaParser;

ElkaError elka_parser_parse(SumkaParser *parser);
void elka_parser_print_error(ElkaParser *parser, SumkaError err);

#endif
