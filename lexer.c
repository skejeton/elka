#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "lexer.h"

#define checkout(x) do { ElkaError err; if((err = (x))) { fprintf(stderr, ">>> %-20s %s:%d\n", __FUNCTION__, __FILE__, __LINE__); return err;} } while (0)

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
ElkaToken mktok(Capture cap, ElkaTokenType tt) {
    return (ElkaToken) { tt, cap.line, cap.column, cap.start, cap.size };
}

static
Capture record(ElkaLexer *lexer) {
    return (Capture) { lexer->line_, lexer->column_, lexer->pos_, 0 };
}

static
void stop(ElkaLexer *lexer, Capture *cap) {
    assert(lexer->pos_ >= cap->start && "Capture is below lexer position");
    cap->size = lexer->pos_-cap->start;
}

static 
bool verify(ElkaLexer *lexer, Capture *cap, const char *against) {
    return strncmp(lexer->source+cap->start, against, cap->size) == 0;
}

static
char lpeek(ElkaLexer *lexer) {
    return lexer->source[lexer->pos_];
}

static
char lis(ElkaLexer *lexer, char chara) {
    return lexer->source[lexer->pos_] == chara;
}

static
char lmeets(ElkaLexer *lexer, const char *str) {
    return strncmp(str, lexer->source+lexer->pos_, strlen(str)) == 0;
}

static
char lnot(ElkaLexer *lexer, char chara) {
    return lexer->source[lexer->pos_] != chara;
}

static
void lskip(ElkaLexer *lexer) {
    lexer->column_ += 1;
    
    if (lis(lexer, '\n')) {
        lexer->column_ = 0;
        lexer->line_ += 1;
    }
       
    lexer->pos_ += 1;
}

static
ElkaError get_ident(ElkaLexer *lexer, Capture *capture) {
    *capture = record(lexer);
    
    /* NOTE: isalnum() fits here because decide() checks for first 
     *       character to be an identifier
     */
    while (isalnum(lpeek(lexer)) || lis(lexer, '_')) 
        lskip(lexer);
    
    stop(lexer, capture);
    return ELKA_OK;
}

static
ElkaError ident_or_kw(ElkaLexer *lexer, ElkaToken *out) {
    Capture capture;
    checkout(get_ident(lexer, &capture));
    
    if (verify(lexer, &capture, "fn"))
        *out = mktok(capture, ELKA_TT_KW_FN);
    else if (verify(lexer, &capture, "if"))
        *out = mktok(capture, ELKA_TT_IF);
    else if (verify(lexer, &capture, "for"))
        *out = mktok(capture, ELKA_TT_FOR);
    else if (verify(lexer, &capture, "return"))
        *out = mktok(capture, ELKA_TT_RETURN);
    else
        *out = mktok(capture, ELKA_TT_IDENT);
    return ELKA_OK;
}

static
ElkaError simple(ElkaLexer *lexer, ElkaTokenType tt, ElkaToken *token) {
    *token = (ElkaToken) {
        tt,
        lexer->line_,
        lexer->column_,
        lexer->pos_,
        1
    };
    
    lskip(lexer);
    return ELKA_OK;
}

static
ElkaError string(ElkaLexer *lexer, ElkaToken *token) {
    Capture capture = record(lexer);
    // Skip the '"'
    lskip(lexer);
    while (lnot(lexer, '"'))
        lskip(lexer);
    // Skip the closing '"'
    lskip(lexer);
    stop(lexer, &capture);
    *token = mktok(capture, ELKA_TT_STRING);
    return ELKA_OK;
}

static
ElkaError number(ElkaLexer *lexer, ElkaToken *out) {
    Capture capture = record(lexer);
    if (lpeek(lexer) == '-')
        lskip(lexer);
    while (lpeek(lexer) >= '0' && lpeek(lexer) <= '9')
        lskip(lexer);
    stop(lexer, &capture);
    *out = mktok(capture, ELKA_TT_NUMBER);
    return ELKA_OK;
}

static
ElkaError decide(ElkaLexer *lexer, ElkaToken *out) {
    char chara = lpeek(lexer);
    
    if (isalpha(chara))
        return ident_or_kw(lexer, out);
    if (lis(lexer, '"'))
        return string(lexer, out);
    if (lis(lexer, ','))
        return simple(lexer, ELKA_TT_COMMA, out);
    if (lis(lexer, '('))
        return simple(lexer, ELKA_TT_LPAREN, out);
    if (lis(lexer, ')'))
        return simple(lexer, ELKA_TT_RPAREN, out);
    if (lis(lexer, '{'))
        return simple(lexer, ELKA_TT_LBRACE, out);
    if (lis(lexer, '}'))
        return simple(lexer, ELKA_TT_RBRACE, out);
    if (lis(lexer, '['))
        return simple(lexer, ELKA_TT_LBRACKET, out);
    if (lis(lexer, ']'))
        return simple(lexer, ELKA_TT_RBRACKET, out);
    if (lis(lexer, '*'))
        return simple(lexer, ELKA_TT_ASTERISK, out);
    if (lis(lexer, '='))
        return simple(lexer, ELKA_TT_ASSIGN, out);
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
            *out = mktok(capture, ELKA_TT_DEFN);
        }
        else {
            stop(lexer, &capture);
            *out = mktok(capture, ELKA_TT_COLON);
        }
        return ELKA_OK;
    }

    return ELKA_ERR_INVALID_CHARACTER;
}

static
bool skip_comment(ElkaLexer *lexer) {
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
void skip_blank(ElkaLexer *lexer) {
    while (lis(lexer, ' ') || lis(lexer, '\t') ||
           lis(lexer, '\r') || lis(lexer, '\n') || skip_comment(lexer))
    {
        lskip(lexer);
    }
}

ElkaError elka_lex_next(ElkaLexer *lexer, ElkaToken *out) {
    skip_blank(lexer);
    if (lpeek(lexer) == 0)
        return ELKA_ERR_EOF;
    return decide(lexer, out);
}

void elka_token_dbgdmp(ElkaLexer *lexer, ElkaToken *token) {
    printf("tok (%d) '%.*s' %d:%d\n", 
        token->type,
        (int)token->size, lexer->source+token->start,
        token->line, token->column); 
}

void elka_token_valdmp(ElkaLexer *lexer, ElkaToken *token) {
    printf("%.*s", 
        (int)token->size, lexer->source+token->start);
}
