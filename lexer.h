#ifndef SUMKA_LEXER_H__
#define SUMKA_LEXER_H__
#include <stdint.h>
#include <stddef.h>
#include "general.h"

typedef enum {
    SUMKA_TT_IDENT,
    SUMKA_TT_KW_FN,
    SUMKA_TT_COLON,
    SUMKA_TT_LPAREN,
    SUMKA_TT_RPAREN,
    SUMKA_TT_LBRACE,
    SUMKA_TT_RBRACE,
    SUMKA_TT_STRING,
    SUMKA_TT_NUMBER,
    SUMKA_TT_DEFN,
    SUMKA_TT_COMMA,
    SUMKA_TT_RETURN
}
SumkaTokenType;

typedef struct SumkaToken {
    SumkaTokenType type;

    int32_t line;
    int32_t column;
    
    // Specifies starting and ending
    // point of token, as according to
    // the source code
    size_t start;
    size_t size;
} SumkaToken;

// Lexer state, construct by passing
// any field except the ones with trailing underscore
typedef struct SumkaLexer {
    const char *source;
    
    size_t pos_;
    int32_t line_;
    int32_t column_;
} SumkaLexer;

// Fetches next token
SumkaError sumka_lex_next(SumkaLexer *lexer, SumkaToken *out_token);

// Returns the line of the token (1-indexed)
int32_t sumka_token_line(SumkaToken *token);

// Returns the column of the token (1-indexed)
int32_t sumka_token_column(SumkaToken *token);

// Outputs token into stdout, for debugging purposes
void sumka_token_dbgdmp(SumkaLexer *lexer, SumkaToken *token);

// Outputs just the contents of the token, for debugging purposes
void sumka_token_valdmp(SumkaLexer *lexer, SumkaToken *token);

#endif
