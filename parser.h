#ifndef SUMKA_PARSER_H__
#define SUMKA_PARSER_H__
#include "lexer.h"
#include "codegen.h"
#include "ffi.h"
#include <stdbool.h>

typedef struct SumkaParserError {
    // FIXME: For now we're just gonna store text, yeah
    char txt[1024];
} SumkaParserError;

typedef struct SumkaParser {
    bool eof;
    SumkaLexer *lexer;
    SumkaCodegen cg;
    SumkaToken current_;
    char tmpstrbuf_[1024];
    SumkaRefl *refl;
    
    // I should probably move this to reflection
    SumkaReflItem *last_type;
    SumkaReflItem *return_type;
    
    SumkaParserError err;
} SumkaParser;

SumkaError sumka_parser_parse(SumkaParser *parser);
void sumka_parser_print_error(SumkaParser *parser, SumkaError err);

#endif
