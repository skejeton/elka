#ifndef SUMKA_PARSER_H__
#define SUMKA_PARSER_H__
#include "lexer.h"
#include "codegen.h"
#include "ffi.h"
#include "typesystem.h"
#include <stdbool.h>

typedef struct SumkaLookupSlot {
    char *name;
    SumkaType type;
} SumkaLookupSlot;

// Slot for variable lookup
typedef struct SumkaLookup {
    SumkaLookupSlot slots[1024];
    size_t slot_count;
} SumkaLookup;

typedef struct SumkaParser {
    bool eof;
    SumkaLexer *lexer;
    SumkaFFI *ffi;
    SumkaCodegen cg;
    SumkaToken current_;
    char tmpstrbuf_[1024];
    SumkaLookup lookup;
    SumkaType last_type;
} SumkaParser;

SumkaError sumka_parser_parse(SumkaParser *parser);

#endif
