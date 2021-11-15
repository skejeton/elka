#include "parser.h"
#include "lexer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#define checkout(x) do { SumkaError err; if((err = (x))) { fprintf(stderr, ">>> %-20s %s:%d\n", __FUNCTION__, __FILE__, __LINE__); return err;} } while (0)

static
SumkaError pskip(SumkaParser *parser) {
    checkout(sumka_lex_next(parser->lexer, &parser->current_));
    return SUMKA_OK;
}

static
bool pcmp(SumkaParser *parser, const char *against) {
    if (parser->eof)
        return false;
    return strncmp(parser->lexer->source+parser->current_.start, against, parser->current_.size) == 0;
}

static
bool pif(SumkaParser *parser, SumkaTokenType tt) {
    if (parser->eof)
        return false;
    return parser->current_.type == tt;
}

// This is needed because EOF test always needs to fail
static
bool pifnt(SumkaParser *parser, SumkaTokenType tt) {
    if (parser->eof)
        return false;
    return parser->current_.type != tt;
}

static
SumkaError pthen(SumkaParser *parser, SumkaTokenType tt) {
    if (parser->eof)
        return SUMKA_ERR_EOF;
    if (parser->current_.type == tt) {
        parser->eof = sumka_lex_next(parser->lexer, &parser->current_) == SUMKA_ERR_EOF;
        return SUMKA_OK;
    } 
    return SUMKA_ERR_INVALID_TOKEN;
}

static
size_t slotalloc(SumkaParser *parser, char *name, SumkaType type) {
    parser->lookup.slots[parser->lookup.slot_count].name = name;
    parser->lookup.slots[parser->lookup.slot_count].type = type;
    return parser->lookup.slot_count++;
}

static
size_t findslot(SumkaParser *parser, char *name) {
    for (int i = parser->lookup.slot_count-1; i >= 0; i -= 1) {
        if (strcmp(parser->lookup.slots[i].name, name) == 0)
            return (size_t)i;
    } 
    // FIXME: NOPE, use SumkaError, for gods sake
    fprintf(stderr, "There's no such variable %s\n", name);
    exit(-1);
}

static
size_t frame(SumkaParser *parser) {
    return parser->lookup.slot_count;
}

static
void restore(SumkaParser *parser, size_t current_frame) {
    parser->lookup.slot_count = current_frame;
}

static
SumkaError pgive(SumkaParser *parser, SumkaTokenType tt, SumkaToken *token) {
    if (parser->eof)
        return SUMKA_ERR_EOF;
    if (parser->current_.type == tt) {
        *token = parser->current_;
        parser->eof = sumka_lex_next(parser->lexer, &parser->current_) == SUMKA_ERR_EOF;
        return SUMKA_OK;
    } 
    return SUMKA_ERR_INVALID_TOKEN;
}

static
char* strmk(SumkaParser *parser, SumkaToken *token) {
    memcpy(parser->tmpstrbuf_, parser->lexer->source+token->start, token->size);
    // Nul terminator
    parser->tmpstrbuf_[token->size] = 0;
    return parser->tmpstrbuf_;
}

/* 
 * FIXME: This currently doesn't handle escape sequences, like 
 *        \n, \t, \\ and so on (\" will be resolved in tokenizer)
 */
static
char* stresc(SumkaParser *parser, SumkaToken *token) {
    memcpy(parser->tmpstrbuf_, parser->lexer->source+token->start+1, token->size-2);
    // Nul terminator
    parser->tmpstrbuf_[token->size-2] = 0;
    return parser->tmpstrbuf_;
}

/* FIXME: This should ideally inform the parser of the size of the final
 *        value being pushed on the stack.  
 */
static
SumkaError par_value(SumkaParser *parser) {
    if (pif(parser, SUMKA_TT_STRING)) {
        SumkaToken value;
        checkout(pgive(parser, SUMKA_TT_STRING, &value));
        char *escaped = stresc(parser, &value);
        sumka_codegen_instr_sc(&parser->cg, SUMKA_INSTR_PUSH_SC, escaped);   
        parser->last_type = (SumkaType) { SUMKA_TYPE_STR };
    }
    else if (pif(parser, SUMKA_TT_NUMBER)) {
        SumkaToken value;
        checkout(pgive(parser, SUMKA_TT_NUMBER, &value));
        char *end = 0;
        sumka_default_int_td num = strtoll(parser->lexer->source+value.start, &end, 10);
        if (errno == ERANGE) 
            return SUMKA_ERR_NUMBER_OUT_OF_RANGE;
        sumka_codegen_instr_ic(&parser->cg, SUMKA_INSTR_PUSH_IC, num);
        parser->last_type = (SumkaType) { SUMKA_TYPE_INT };
    }
    else if (pif(parser, SUMKA_TT_IDENT)) {
        SumkaToken value;
        checkout(pgive(parser, SUMKA_TT_IDENT, &value));
        size_t slot = findslot(parser, strmk(parser, &value));
        sumka_codegen_instr_iuc(&parser->cg, SUMKA_INSTR_LOAD_IUC, slot);
        parser->last_type = parser->lookup.slots[slot].type;
    }

    return SUMKA_OK;
}

static
SumkaError par_delimited(SumkaParser *parser) {
    /* FIXME: The parser will catch closing parentheses,
     *        what if we need to parse curly-brace delimited?
     */
    if (pif(parser, SUMKA_TT_RPAREN))
        return SUMKA_OK;

    /* FIXME: This only calls par_value, which might be reasonable
     *        but only for now
     */
    checkout(par_value(parser));
    
    return SUMKA_OK;
}

static
SumkaError emit_call(SumkaParser *parser, SumkaToken id) {
    const char *need = parser->lexer->source+id.start;
    /* FIXME: Use hash table, not scan everything every time, dangit!
     */
    for (size_t i = 0; i < parser->ffi->mapping_count; i += 1) {
        const char *got = parser->ffi->mappings[i].name;
        if (strncmp(need, got, id.size) == 0) {
            sumka_codegen_instr_iuc(&parser->cg, SUMKA_INSTR_CALL_FFI_IUC, i);
            return SUMKA_OK;
        }
    }
    sumka_codegen_instr_sc(&parser->cg, SUMKA_INSTR_CALL_SC, strmk(parser, &id));

    /* FIXME: None of this just makes any goddamn sense
     */
    return SUMKA_OK;

    fprintf(stderr, "There's no such function\n");
    exit(-1);
}

static
SumkaError par_call(SumkaToken name, SumkaParser *parser) {
    checkout(pthen(parser, SUMKA_TT_LPAREN));    
    checkout(par_delimited(parser));    
    checkout(pthen(parser, SUMKA_TT_RPAREN));
 
    checkout(emit_call(parser, name));
    
    return SUMKA_OK;
}

/* FIXME: Related to the par_value() problem, this should somehow work together
 *        with types
 */
static
SumkaError par_defn(SumkaToken name, SumkaParser *parser) {
    checkout(pthen(parser, SUMKA_TT_DEFN));    
    checkout(par_value(parser));    
    slotalloc(parser, strmk(parser, &name), parser->last_type);
    return SUMKA_OK;
}

static
SumkaError decide_statement(SumkaParser *parser) {
    SumkaToken name;
    checkout(pgive(parser, SUMKA_TT_IDENT, &name));


    if (pif(parser, SUMKA_TT_LPAREN))
        checkout(par_call(name, parser));
    else if (pif(parser, SUMKA_TT_DEFN))
        checkout(par_defn(name, parser));
    else
        /* FIXME: Removing this placeholder
         */
        checkout(SUMKA_ERR_PLACEHOLDER);
    return SUMKA_OK;
}

static
SumkaError par_body(SumkaParser *parser) {
    checkout(pthen(parser, SUMKA_TT_LBRACE));
    while (pifnt(parser, SUMKA_TT_RBRACE))
        checkout(decide_statement(parser));        
    checkout(pthen(parser, SUMKA_TT_RBRACE));

    return SUMKA_OK;
}

static
SumkaError par_fn(SumkaParser *parser) {
    size_t current_frame = frame(parser);
    checkout(pthen(parser, SUMKA_TT_KW_FN));
    SumkaToken name;
    checkout(pgive(parser, SUMKA_TT_IDENT, &name));
    
    SumkaLabel *lbl = sumka_codegen_label(&parser->cg, strmk(parser, &name));
    
    checkout(pthen(parser, SUMKA_TT_LPAREN));
    // We have a parameter
    // FIXME: Support for multliple parameters
    if (pifnt(parser, SUMKA_TT_RPAREN)) {
        SumkaToken param_name;
        checkout(pgive(parser, SUMKA_TT_IDENT, &param_name));
        checkout(pthen(parser, SUMKA_TT_COLON));
        lbl->has_arg = true;
        if (pcmp(parser, "str"))
            lbl->type = (SumkaType){SUMKA_TYPE_STR};
        else if (pcmp(parser, "int"))
            lbl->type = (SumkaType){SUMKA_TYPE_INT};
        else 
            return SUMKA_ERR_UNKNOWN_TYPE;
        slotalloc(parser, strmk(parser, &name), lbl->type);
        checkout(pskip(parser));
    }
    checkout(pthen(parser, SUMKA_TT_RPAREN));
    checkout(par_body(parser));
 
    sumka_codegen_instr(&parser->cg, SUMKA_INSTR_RETN);   
    
    restore(parser, current_frame);
    return SUMKA_OK;
}

SumkaError sumka_parser_parse(SumkaParser *parser) {
    checkout(sumka_lex_next(parser->lexer, &parser->current_));
    while(!parser->eof) checkout(par_fn(parser));
    return SUMKA_OK;
}


