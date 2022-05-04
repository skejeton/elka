#ifndef ELKA_LEXER_H__
#define ELKA_LEXER_H__
#include <stdint.h>
#include <stddef.h>
#include "general.h"

typedef enum {
    ELKA_TT_IDENT,
    ELKA_TT_KW_FN,
    ELKA_TT_COLON,
    ELKA_TT_LPAREN,
    ELKA_TT_RPAREN,
    ELKA_TT_LBRACE,
    ELKA_TT_RBRACE,
    ELKA_TT_STRING,
    ELKA_TT_NUMBER,
    ELKA_TT_DEFN,
    ELKA_TT_COMMA,
    ELKA_TT_RETURN,
    ELKA_TT_IF,
    ELKA_TT_ASSIGN,
    ELKA_TT_FOR,
    ELKA_TT_LBRACKET,
    ELKA_TT_RBRACKET,
		ELKA_TT_ASTERISK,
		ELKA_TT_CARET,
		ELKA_TT_SLASH,
		ELKA_TT_OP_MODULO,
		ELKA_TT_OP_SHIFTL,
		ELKA_TT_OP_SHIFTR,
		ELKA_TT_OP_EQUAL,
		ELKA_TT_OP_NOT_EQUAL,
		ELKA_TT_OP_LESSER,
		ELKA_TT_OP_LESSER_EQUAL,
		ELKA_TT_OP_GREATER,
		ELKA_TT_OP_GREATER_EQUAL,
		ELKA_TT_OP_AND,
		ELKA_TT_OP_OR,
		ELKA_TT_OP_PLUS,
		ELKA_TT_OP_MINUS,
		ELKA_TT_OP_NOT,
		ELKA_TT_OP_BITNOT,
		ELKA_TT_OP_BITAND,
		ELKA_TT_OP_BITOR,
		ELKA_TT_EOF,
}
ElkaTokenType;

typedef struct ElkaToken {
    ElkaTokenType type;

    int32_t line;
    int32_t column;
    
    // Specifies starting and ending
    // point of token, as according to
    // the source code
    size_t start;
    size_t size;
} ElkaToken;

// Lexer state, construct by passing
// any field except the ones with trailing underscore
typedef struct ElkaLexer {
    const char *source;
    
    size_t pos_;
    int32_t line_;
    int32_t column_;
} ElkaLexer;

// Fetches next token
ElkaError elka_lex_next(ElkaLexer *lexer, ElkaToken *out_token);

// Returns the next token, but doesn't advance the lexer position
ElkaError elka_lex_peek(ElkaLexer *lexer, ElkaToken *out_token);

// Returns the line of the token (1-indexed)
int32_t elka_token_line(ElkaToken *token);

// Returns the column of the token (1-indexed)
int32_t elka_token_column(ElkaToken *token);

// Outputs token into stdout, for debugging purposes
void elka_token_dbgdmp(ElkaLexer *lexer, ElkaToken *token);

// Outputs just the contents of the token, for debugging purposes
void elka_token_valdmp(ElkaLexer *lexer, ElkaToken *token);

#endif
