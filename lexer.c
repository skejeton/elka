#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "lexer.h"

#define checkout(x) do { SumkaError err; if((err = (x))) { fprintf(stderr, ">>> %-20s %s:%d\n", __FUNCTION__, __FILE__, __LINE__); return err;} } while (0)

// Captures a certain span 
typedef struct Capture {
    // The starting line of the capture
    int32_t line;
    // The starting column of the capture
    int32_t column;
    size_t start;
    size_t size;
} Capture;

static
SumkaToken mktok(Capture cap, SumkaTokenType tt) {
    return (SumkaToken) { tt, cap.line, cap.column, cap.start, cap.size };
}

static
Capture record(SumkaLexer *lexer) {
    return (Capture) { lexer->line_, lexer->column_, lexer->pos_, 0 };
}

static
void stop(SumkaLexer *lexer, Capture *cap) {
    assert(lexer->pos_ >= cap->start && "Capture is below lexer position");
    cap->size = lexer->pos_-cap->start;
}

static 
bool verify(SumkaLexer *lexer, Capture *cap, const char *against) {
    return strncmp(lexer->source+cap->start, against, cap->size) == 0;
}

static
char lpeek(SumkaLexer *lexer) {
    return lexer->source[lexer->pos_];
}

static
char lis(SumkaLexer *lexer, char chara) {
    return lexer->source[lexer->pos_] == chara;
}

static
char lmeets(SumkaLexer *lexer, const char *str) {
    return strncmp(str, lexer->source+lexer->pos_, strlen(str)) == 0;
}

static
char lnot(SumkaLexer *lexer, char chara) {
    return lexer->source[lexer->pos_] != chara;
}

static
void lskip(SumkaLexer *lexer) {
    lexer->column_ += 1;
    
    if (lis(lexer, '\n')) {
        lexer->column_ = 0;
        lexer->line_ += 1;
    }
       
    lexer->pos_ += 1;
}

static
SumkaError get_ident(SumkaLexer *lexer, Capture *capture) {
    *capture = record(lexer);
    
    /* NOTE: isalnum() fits here because decide() checks for first 
     *       character to be an identifier
     */
    while (isalnum(lpeek(lexer)) || lis(lexer, '_')) 
        lskip(lexer);
    
    stop(lexer, capture);
    return SUMKA_OK;
}

static
SumkaError ident_or_kw(SumkaLexer *lexer, SumkaToken *out) {
    Capture capture;
    checkout(get_ident(lexer, &capture));
    
    if (verify(lexer, &capture, "fn"))
        *out = mktok(capture, SUMKA_TT_KW_FN);
    else if (verify(lexer, &capture, "if"))
        *out = mktok(capture, SUMKA_TT_IF);
    else if (verify(lexer, &capture, "for"))
        *out = mktok(capture, SUMKA_TT_FOR);
    else if (verify(lexer, &capture, "return"))
        *out = mktok(capture, SUMKA_TT_RETURN);
    else
        *out = mktok(capture, SUMKA_TT_IDENT);
    return SUMKA_OK;
}

static
SumkaError simple(SumkaLexer *lexer, SumkaTokenType tt, SumkaToken *token) {
    *token = (SumkaToken) {
        tt,
        lexer->line_,
        lexer->column_,
        lexer->pos_,
        1
    };
    
    lskip(lexer);
    return SUMKA_OK;
}

static
SumkaError string(SumkaLexer *lexer, SumkaToken *token) {
    Capture capture = record(lexer);
    // Skip the '"'
    lskip(lexer);
    while (lnot(lexer, '"'))
        lskip(lexer);
    // Skip the closing '"'
    lskip(lexer);
    stop(lexer, &capture);
    *token = mktok(capture, SUMKA_TT_STRING);
    return SUMKA_OK;
}

static
SumkaError number(SumkaLexer *lexer, SumkaToken *out) {
    Capture capture = record(lexer);
    if (lpeek(lexer) == '-')
        lskip(lexer);
    while (lpeek(lexer) >= '0' && lpeek(lexer) <= '9')
        lskip(lexer);
    stop(lexer, &capture);
    *out = mktok(capture, SUMKA_TT_NUMBER);
    return SUMKA_OK;
}

static
SumkaError decide(SumkaLexer *lexer, SumkaToken *out) {
    char chara = lpeek(lexer);
    
    if (isalpha(chara))
        return ident_or_kw(lexer, out);
    if (lis(lexer, '"'))
        return string(lexer, out);
    if (lis(lexer, ','))
        return simple(lexer, SUMKA_TT_COMMA, out);
    if (lis(lexer, '('))
        return simple(lexer, SUMKA_TT_LPAREN, out);
    if (lis(lexer, ')'))
        return simple(lexer, SUMKA_TT_RPAREN, out);
    if (lis(lexer, '{'))
        return simple(lexer, SUMKA_TT_LBRACE, out);
    if (lis(lexer, '}'))
        return simple(lexer, SUMKA_TT_RBRACE, out);
    if (lis(lexer, '['))
        return simple(lexer, SUMKA_TT_LBRACKET, out);
    if (lis(lexer, ']'))
        return simple(lexer, SUMKA_TT_RBRACKET, out);
    if (lis(lexer, '='))
        return simple(lexer, SUMKA_TT_ASSIGN, out);
    if (lpeek(lexer) == '-' || (lpeek(lexer) >= '0' && lpeek(lexer) <= '9'))
        return number(lexer, out);
    /* FIXME: This isn't a very good way to handle multicharacter seq's
     *        i probably need to separate it
     */

    // := Should be treated as : and = not a single thing!
    Capture capture = record(lexer);
    if (lis(lexer, ':')) {
        lskip(lexer);
        if (lis(lexer, '=')) {
            lskip(lexer);
            stop(lexer, &capture);
            *out = mktok(capture, SUMKA_TT_DEFN);
        }
        else {
            stop(lexer, &capture);
            *out = mktok(capture, SUMKA_TT_COLON);
        }
        return SUMKA_OK;
    }

    return SUMKA_ERR_INVALID_CHARACTER;
}

static
bool skip_comment(SumkaLexer *lexer) {
    if (lmeets(lexer, "/*")) {
        lskip(lexer);
        lskip(lexer);
        while (!lmeets(lexer, "*/")) {
            lskip(lexer);
        }
        lskip(lexer);
        lskip(lexer);
        return true;
    }
    return false;
}

static
void skip_blank(SumkaLexer *lexer) {
    while (lis(lexer, ' ') || lis(lexer, '\t') ||
           lis(lexer, '\r') || lis(lexer, '\n') || skip_comment(lexer))
    {
        lskip(lexer);
    }
}

SumkaError sumka_lex_next(SumkaLexer *lexer, SumkaToken *out) {
    skip_blank(lexer);
    if (lpeek(lexer) == 0)
        return SUMKA_ERR_EOF;
    return decide(lexer, out);
}

void sumka_token_dbgdmp(SumkaLexer *lexer, SumkaToken *token) {
    printf("tok (%d) '%.*s' %d:%d\n", 
        token->type,
        (int)token->size, lexer->source+token->start,
        token->line, token->column); 
}

void sumka_token_valdmp(SumkaLexer *lexer, SumkaToken *token) {
    printf("%.*s", 
        (int)token->size, lexer->source+token->start);
}
