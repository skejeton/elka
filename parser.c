#include "parser.h"
#include "lexer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#define checkout(x) do { SumkaError err; if((err = (x))) { fprintf(stderr, ">>> %d %-20s %s:%d\n", err, __FUNCTION__, __FILE__, __LINE__); return err;} } while (0)

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

/*
static
void message(SumkaParser *parser, const char *message) {
    strcpy(parser->err.txt, message);
}
*/

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

static
SumkaError par_call(SumkaToken name, SumkaParser *parser);

static
SumkaError par_value(SumkaParser *parser) {
    if (pif(parser, SUMKA_TT_STRING)) {
        SumkaToken value;
        checkout(pgive(parser, SUMKA_TT_STRING, &value));
        char *escaped = stresc(parser, &value);
        sumka_codegen_instr_sc(&parser->cg, SUMKA_INSTR_PUSH_SC, escaped);   
        parser->last_type = SUMKA_TYPE_STR;
    }
    else if (pif(parser, SUMKA_TT_NUMBER)) {
        SumkaToken value;
        checkout(pgive(parser, SUMKA_TT_NUMBER, &value));
        char *end = 0;
        sumka_default_int_td num = strtoll(parser->lexer->source+value.start, &end, 10);
        if (errno == ERANGE) 
            return SUMKA_ERR_NUMBER_OUT_OF_RANGE;
        sumka_codegen_instr_ic(&parser->cg, SUMKA_INSTR_PUSH_IC, num);
        parser->last_type = SUMKA_TYPE_INT;
    }
    else if (pif(parser, SUMKA_TT_IDENT)) {
        SumkaToken value;
        checkout(pgive(parser, SUMKA_TT_IDENT, &value));
        if (pif(parser, SUMKA_TT_LPAREN)) {
            checkout(par_call(value, parser));
            return SUMKA_OK;
        }

        int slot = sumka_refl_lup(&parser->cg.refl, strmk(parser, &value));
        if (slot == -1)
            return SUMKA_ERR_VARIABLE_NOT_FOUND;
        /* XXX: Here we subtract size from slot, this is for relative addressing
         * While this might work for now, this isn't guaranteed to be true in future
         */
        sumka_codegen_instr_iuc(&parser->cg, SUMKA_INSTR_LOAD_IUC, parser->cg.refl.stack_size-(slot+1));
        parser->last_type = parser->cg.refl.refls[slot].type;
    }

    return SUMKA_OK;
}

static
SumkaError par_call_params(SumkaParser *parser, SumkaReflItem *item) {
    if (pif(parser, SUMKA_TT_RPAREN))
        return SUMKA_OK;

    SumkaReflItem *arg = item ? item->fn.first_arg : NULL;
    while (pifnt(parser, SUMKA_TT_RPAREN)) {
        if (item && arg == NULL)
            return SUMKA_ERR_TOO_MANY_ARGS;
        checkout(par_value(parser));
        // FFI calls provide NULL so we don't typecheck here
        if (item)
            if (!sumka_refl_typecheck(arg, parser->last_type))
                return SUMKA_ERR_TYPE_MISMATCH;

        if (pif(parser, SUMKA_TT_RPAREN))
            break;
        if (item)
            arg = arg->sibling;
        checkout(pthen(parser, SUMKA_TT_COMMA));
    }
    if (item && arg->sibling != NULL)
        return SUMKA_ERR_NOT_ENOUGH_ARGS;   
    return SUMKA_OK;
}

static
SumkaError par_call(SumkaToken name, SumkaParser *parser) {
    checkout(pthen(parser, SUMKA_TT_LPAREN));    

    int id = sumka_refl_lup(&parser->cg.refl, strmk(parser, &name));
    if (id != -1) {
        /* FIXME:
         * Here I've caught up with a bug that happened because i accessed stuff directly
         * I should really avoid doing that or it will suck BAD.
         */
        SumkaReflItem *item = parser->cg.refl.stack[id];
        if (item->type != SUMKA_TYPE_FUN)
            return SUMKA_ERR_CALL_ON_NON_FUNCTION;
        
        checkout(par_call_params(parser, item));  
        
        if (item->fn.ret_type)
            parser->last_type = item->fn.ret_type->type;
        sumka_codegen_instr_sc(&parser->cg, SUMKA_INSTR_CALL_SC, strmk(parser, &name));
    }
    else if ((id = sumka_ffi_find(parser->ffi, strmk(parser, &name))) != -1) {
        checkout(par_call_params(parser, NULL));    
        sumka_codegen_instr_iuc(&parser->cg, SUMKA_INSTR_CALL_FFI_IUC, id);
    }
    else {
        return SUMKA_ERR_FUNCTION_NOT_FOUND;
    }
    checkout(pthen(parser, SUMKA_TT_RPAREN));
    
    return SUMKA_OK;
}

/* FIXME: Related to the par_value() problem, this should somehow work together
 *        with types
 */
static
SumkaError par_defn(SumkaToken name, SumkaParser *parser) {
    checkout(pthen(parser, SUMKA_TT_DEFN));    
    checkout(par_value(parser));
    sumka_refl_make_var(&parser->cg.refl, strmk(parser, &name), parser->last_type);
    return SUMKA_OK;
}

static
SumkaError decide_statement(SumkaParser *parser) {
    SumkaToken name;
    if (pif(parser, SUMKA_TT_RETURN)) {
        checkout(pskip(parser));
        sumka_codegen_instr(&parser->cg, SUMKA_INSTR_CLR);   
        checkout(par_value(parser));
        if (parser->last_type != parser->return_type)
            return SUMKA_ERR_TYPE_MISMATCH;
        sumka_codegen_instr(&parser->cg, SUMKA_INSTR_RETN);   
    }
    else {
        checkout(pgive(parser, SUMKA_TT_IDENT, &name));

        if (pif(parser, SUMKA_TT_LPAREN))
            checkout(par_call(name, parser));
        else if (pif(parser, SUMKA_TT_DEFN))
            checkout(par_defn(name, parser));
        else
            /* FIXME: Removing this placeholder
             */
            checkout(SUMKA_ERR_PLACEHOLDER);
    }
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
    checkout(pthen(parser, SUMKA_TT_KW_FN));
    SumkaToken name;
    checkout(pgive(parser, SUMKA_TT_IDENT, &name));
 
    int decl = sumka_refl_lup(&parser->cg.refl, strmk(parser, &name));
    // We don't want to provide definitions for non-functions
    if (decl != -1 && parser->cg.refl.stack[decl]->type != SUMKA_TYPE_FUN)
        decl = -1;

    SumkaReflItem *item;
    if (decl == -1)
        item = sumka_refl_make_fn(&parser->cg.refl, strmk(parser, &name), parser->cg.instr_count);
    else {
        // Definition
        item = parser->cg.refl.stack[decl];
        item->fn.addr = parser->cg.instr_count;
    }

    sumka_refl_push(&parser->cg.refl, item);
    size_t current_frame = sumka_refl_peek(&parser->cg.refl);
    checkout(pthen(parser, SUMKA_TT_LPAREN));

    SumkaReflItem *arg = item->fn.first_arg;

    while (pifnt(parser, SUMKA_TT_RPAREN)) {
        if (decl != -1 && arg == NULL)
            return SUMKA_ERR_TOO_MANY_ARGS;
        SumkaToken param_name;
        checkout(pgive(parser, SUMKA_TT_IDENT, &param_name));
        checkout(pthen(parser, SUMKA_TT_COLON));

        enum SumkaTypeKind kind;

        if (pcmp(parser, "str"))
            kind = SUMKA_TYPE_STR;
        else if (pcmp(parser, "int"))
            kind = SUMKA_TYPE_INT;
        else 
            return SUMKA_ERR_UNKNOWN_TYPE;
        SumkaReflItem *param = sumka_refl_make_var(&parser->cg.refl, strmk(parser, &param_name), kind);
        // Register parameter as variable
        sumka_refl_push(&parser->cg.refl, param);
        // FIXME: add type checking for forward decl's
        if (decl == -1)
            sumka_refl_add_param(item, param);
        else if (!sumka_refl_typecheck(arg, kind))
            return SUMKA_ERR_TYPE_MISMATCH;
        else 
            arg = arg->sibling;

        checkout(pskip(parser));
        if (pif(parser, SUMKA_TT_RPAREN))
            break;
        checkout(pthen(parser, SUMKA_TT_COMMA));
    }
    if (decl != -1 && arg != NULL)
        return SUMKA_ERR_NOT_ENOUGH_ARGS;

    checkout(pthen(parser, SUMKA_TT_RPAREN));

    if (pif(parser, SUMKA_TT_COLON)) {
        checkout(pskip(parser));
        // Too many duplication when we do typechecking, i must fix that!
        enum SumkaTypeKind kind;
        if (pcmp(parser, "str"))
            kind = SUMKA_TYPE_STR;
        else if (pcmp(parser, "int"))
            kind = SUMKA_TYPE_INT;
        else 
            return SUMKA_ERR_UNKNOWN_TYPE;
            
        item->fn.ret_type = sumka_refl_make_var(&parser->cg.refl, "", kind);
            
        parser->return_type = kind;
        checkout(pskip(parser));
    }
    else {
        parser->return_type = SUMKA_TYPE_VOID;
        item->fn.ret_type = sumka_refl_make_var(&parser->cg.refl, "", SUMKA_TYPE_VOID);
    }

    // Is definition or forward decl. 
    if (pif(parser, SUMKA_TT_LBRACE)) {
        checkout(par_body(parser));
        sumka_codegen_instr(&parser->cg, SUMKA_INSTR_CLR);   
        sumka_codegen_instr(&parser->cg, SUMKA_INSTR_RETN);   
        item->present = true;
    }
    else {
        // NOTE: This is to check if there already was a forward declaration
        // Since by default items are present
        // We don't want duplicate forward declarations
        if (!item->present) {
            return SUMKA_ERR_DOUBLE_FORWARD_DECL;
        }
        item->present = false;
    }
    
    sumka_refl_seek(&parser->cg.refl, current_frame);
    return SUMKA_OK;
}

SumkaError sumka_parser_parse(SumkaParser *parser) {
    checkout(sumka_lex_next(parser->lexer, &parser->current_));
    while(!parser->eof) checkout(par_fn(parser));
    return SUMKA_OK;
}


/*
void sumka_parser_print_error(SumkaParser *parser, SumkaError err) {
    const char *err_names[1024] = {
        [SUMKA_ERR_TYPE_MISMATCH] = "Type mismatch",
    };
    printf("Error: %s(%d), at line %d\n", err_names[err], err, parser->current_.line);
    printf("    %s\n", parser->err.txt);
}
*/
