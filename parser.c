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
#define checkout(x) do { SumkaError err; if((err = (x))) { fprintf(stderr, ">>> %d %-20s %s:%d\n", err, __FUNCTION__, __FILE__, __LINE__); return err;} } while (0)
#endif

static
SumkaError pskip(SumkaParser *parser) {
    checkout(sumka_lex_next(parser->lexer, &parser->current_));
    return SUMKA_OK;
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
SumkaError par_value(SumkaParser *parser);

static
SumkaError par_call(SumkaParser *parser);

static
SumkaError par_type(SumkaParser *parser, SumkaReflItem **out_type);

static
SumkaError numerify(SumkaParser *parser, SumkaToken *token, long long *out) {
    char *end = 0;
    sumka_default_int_td num = strtoll(parser->lexer->source+token->start, &end, 10);
    if (errno == ERANGE) 
        return SUMKA_ERR_NUMBER_OUT_OF_RANGE;
    *out = num;
    return SUMKA_OK;
}

static
SumkaError par_access(SumkaParser *parser) {
    checkout(pthen(parser, SUMKA_TT_LBRACKET));

    SumkaReflItem *item = &parser->last_item;
    int id = sumka_refl_lup_id(parser->refl, item->name);

    checkout(par_value(parser));
    
    // TODO: check if index is a number

    // Calculates the relative offset of the variable
    // Note, this is also probably temporary
    int relative = id - parser->stack_base; 
    
    // TODO: Check if whatever we are accessing is an array
    sumka_codegen_instr_iuc(&parser->cg, SUMKA_INSTR_BASED_IUC, relative);
    
    checkout(pthen(parser, SUMKA_TT_RBRACKET));
    
    return SUMKA_OK;
}

static
SumkaError par_atom(SumkaParser *parser) {
    if (pif(parser, SUMKA_TT_STRING)) {
        SumkaToken value;
        checkout(pgive(parser, SUMKA_TT_STRING, &value));
        char *escaped = stresc(parser, &value);
        sumka_codegen_instr_sc(&parser->cg, SUMKA_INSTR_PUSH_SC, escaped);   
        
        // NOTE: We must have a basetype find function
        parser->last_item = sumka_refl_make_dummy(sumka_refl_find(parser->refl, "str"), SUMKA_TAG_VALUE);
    }
    else if (pif(parser, SUMKA_TT_NUMBER)) {
        SumkaToken value;
        checkout(pgive(parser, SUMKA_TT_NUMBER, &value));

        long long num;
        checkout(numerify(parser, &value, &num));     
        
        if (num >= 0 && num < (1 << 25))
            sumka_codegen_instr_iuc(&parser->cg, SUMKA_INSTR_PUSH_IUC_I, num);
        else
            sumka_codegen_instr_ic(&parser->cg, SUMKA_INSTR_PUSH_IC, num);
        
        // NOTE: We must have a basetype find function
        parser->last_item = sumka_refl_make_dummy(sumka_refl_find(parser->refl, "int"), SUMKA_TAG_VALUE);
    }
    else if (pif(parser, SUMKA_TT_IDENT)) {
        SumkaToken value;
        checkout(pgive(parser, SUMKA_TT_IDENT, &value));

        int id = sumka_refl_lup_id(parser->refl, strmk(parser, &value));
        
        if (id == -1)
            return SUMKA_ERR_VARIABLE_NOT_FOUND;
            
        SumkaReflItem *item = parser->refl->stack[id];

        // Calculates the relative offset of the variable
        // Note, this is also probably temporary
        int relative = id - parser->stack_base; 
        
        if (item->tag == SUMKA_TAG_VALUE) {
            sumka_codegen_instr_iuc(&parser->cg, SUMKA_INSTR_LOAD_IUC, relative);
        }
        
        parser->last_item = *item;
    }
    // Array literal
    else if (pif(parser, SUMKA_TT_LBRACKET)) {
        checkout(pskip(parser));
        
        // The token length of our array
        SumkaToken length;
        
        // TODO: Infer the size of the number
        checkout(pgive(parser, SUMKA_TT_NUMBER, &length));
        
        long long length_value;
        checkout(numerify(parser, &length, &length_value));
        
        // Check if the argument passed is positive, we can't have
        // negative sized arrays
        if (length_value < 0) {
            return SUMKA_ERR_NEAGTIVE_SIZED_ARRAY;
        }
        
        checkout(pthen(parser, SUMKA_TT_RBRACKET));
    
        // Type of the array elements
        // TODO: Handle the type here
        SumkaReflItem *type;
        checkout(par_type(parser, &type));
        
        // TODO: Assign array type instead of host type!
        parser->last_item = sumka_refl_make_dummy(type, SUMKA_TAG_VALUE);
        
        checkout(pthen(parser, SUMKA_TT_LBRACE));
        
        long long literal_item_count = 0;
        
        // TODO: Make a generic delimited parser?
        while (pifnt(parser, SUMKA_TT_RBRACE)) {
            checkout(par_value(parser));
            literal_item_count += 1;

            if (pif(parser, SUMKA_TT_RBRACE)) {
                break;
            }
            checkout(pthen(parser, SUMKA_TT_COMMA));
        }
        
        // Verify that amount of items matches the
        // size of the literal specified
        
        if (literal_item_count > length_value) {
            return SUMKA_ERR_LITERAL_EXCESS_ITEMS;
        }
        
        if (literal_item_count < length_value) {
            return SUMKA_ERR_LITERAL_FEW_ITEMS;
        }
        
        checkout(pthen(parser, SUMKA_TT_RBRACE));
    }
    
    return SUMKA_OK;
}

static
SumkaError par_value(SumkaParser *parser) {
    checkout(par_atom(parser));
    if (pif(parser, SUMKA_TT_LPAREN)) {
        checkout(par_call(parser));
        return SUMKA_OK;
    }
    if (pif(parser, SUMKA_TT_LBRACKET)) {
        checkout(par_access(parser));
        return SUMKA_OK;
    }
    return SUMKA_OK;
}

static
SumkaError par_call_params(SumkaParser *parser, SumkaReflItem *item) {
    SumkaReflItem *arg = item ? item->fn.first_arg : NULL;

    while (pifnt(parser, SUMKA_TT_RPAREN)) {
        if (item && arg == NULL)
            return SUMKA_ERR_TOO_MANY_ARGS;

        checkout(par_value(parser));
        
        // FFI calls provide NULL so we don't typecheck here
        if (item)
            if (!sumka_refl_instanceof(arg, parser->last_item.type))
                return SUMKA_ERR_TYPE_MISMATCH;

        if (pif(parser, SUMKA_TT_RPAREN))
            break;
            
        if (item)
            arg = arg->sibling;
        checkout(pthen(parser, SUMKA_TT_COMMA));
    }
    if (item && arg && arg->sibling != NULL)
        return SUMKA_ERR_NOT_ENOUGH_ARGS;   
    return SUMKA_OK;
}

// NOTE: out_type is guaranteed to be initialized so long the return value
//       of the function is SUMKA_OK
static 
SumkaError par_type(SumkaParser *parser, SumkaReflItem **out_type) {
    *out_type = NULL;

    SumkaToken type_name;
    checkout(pgive(parser, SUMKA_TT_IDENT, &type_name));
    SumkaReflItem *type = sumka_refl_lup(parser->refl, strmk(parser, &type_name));
    
    if (!type) 
        return SUMKA_ERR_TYPE_NOT_FOUND;
    
    // For now we are only checking for basetypes, but in the future if we
    // want to add custom types (like structs) this will be changed
    if (type->tag != SUMKA_TAG_BASETYPE)
        return SUMKA_ERR_INVALID_TYPE;

    *out_type = type;
    return SUMKA_OK;
}


static
SumkaError par_call(SumkaParser *parser) {
    checkout(pthen(parser, SUMKA_TT_LPAREN));    

    char *strname = parser->last_item.name;

    if (strcmp(strname, "less") == 0) {
        checkout(par_call_params(parser, NULL));  
        sumka_codegen_instr(&parser->cg, SUMKA_INSTR_LESS);
        checkout(pthen(parser, SUMKA_TT_RPAREN));
        return SUMKA_OK;
    }
    else if (strcmp(strname, "plus") == 0) {
        checkout(par_call_params(parser, NULL));  
        sumka_codegen_instr(&parser->cg, SUMKA_INSTR_ADD);
        checkout(pthen(parser, SUMKA_TT_RPAREN));
        return SUMKA_OK;
    }
    else if (strcmp(strname, "minus") == 0) {
        checkout(par_call_params(parser, NULL));  
        sumka_codegen_instr(&parser->cg, SUMKA_INSTR_SUB);
        checkout(pthen(parser, SUMKA_TT_RPAREN));
        return SUMKA_OK;
    }
    SumkaReflItem *callee = &parser->last_item;
    
    if (callee) {
        if (callee->tag == SUMKA_TAG_FUN) {
            checkout(par_call_params(parser, callee));  
            sumka_codegen_instr_sc(&parser->cg, SUMKA_INSTR_CALL_SC, callee->name);

            parser->last_item = *callee;
        }
        else if (callee->tag == SUMKA_TAG_FFIFUN) {
            int callee_id = sumka_refl_lup_id(parser->refl, callee->name);
            assert(callee_id != -1);
            checkout(par_call_params(parser, NULL));  
            // NOTE: We need to find address of the callee but since it's inside the Parser
            //       it's not really anywhere on the stack of the reflection
            sumka_codegen_instr_iuc(&parser->cg, SUMKA_INSTR_CALL_FFI_IUC, callee_id);
        }
        else {
            return SUMKA_ERR_CALL_ON_NON_FUNCTION;
        }
    }
    else {
        return SUMKA_ERR_FUNCTION_NOT_FOUND;
    }
    checkout(pthen(parser, SUMKA_TT_RPAREN));
    
    return SUMKA_OK;
}

static
SumkaError par_defn(SumkaToken name, SumkaParser *parser) {
    checkout(pthen(parser, SUMKA_TT_DEFN));    
    checkout(par_value(parser));
    sumka_refl_push(parser->refl,
        sumka_refl_make_var(parser->refl, strmk(parser, &name), parser->last_item.type));
    return SUMKA_OK;
}

static 
SumkaError par_body(SumkaParser *parser);

static
SumkaError decide_statement(SumkaParser *parser) {
    SumkaToken name;
    if (pif(parser, SUMKA_TT_IF)) {
        checkout(pskip(parser));
        checkout(par_value(parser));
        size_t genesis = sumka_codegen_branch(&parser->cg);
        checkout(par_body(parser));
        sumka_codegen_leave(&parser->cg, genesis);
    }
    else if (pif(parser, SUMKA_TT_FOR)) {
        checkout(pskip(parser));
        size_t loopback = parser->cg.instr_count;
        checkout(par_value(parser));
        size_t genesis = sumka_codegen_branch(&parser->cg);
        checkout(par_body(parser));
        // FIXME: This is bad, I should make unconditional goto instead
        sumka_codegen_instr_iuc(&parser->cg, SUMKA_INSTR_PUSH_IUC_I, 1);
        sumka_codegen_instr_iuc(&parser->cg, SUMKA_INSTR_GIF_IUC, parser->cg.instr_count-loopback);
        sumka_codegen_leave(&parser->cg, genesis);
    }
    else if (pif(parser, SUMKA_TT_RETURN)) {
        checkout(pskip(parser));
        checkout(par_value(parser));
        if (parser->last_item.type != parser->return_type)
            return SUMKA_ERR_TYPE_MISMATCH;
        if (parser->return_type != sumka_refl_find(parser->refl, "void"))
            sumka_codegen_instr(&parser->cg, SUMKA_INSTR_CLR);   
        sumka_codegen_instr(&parser->cg, SUMKA_INSTR_RETN);   
    }
    else {
        // I shouldn't be doing this
        // This is needed because i have to backtrack if i don't guess assignment
        // or definition
        SumkaLexer lexer = *parser->lexer;
        SumkaParser prev_parser = *parser;

        checkout(pgive(parser, SUMKA_TT_IDENT, &name));

        if (pif(parser, SUMKA_TT_ASSIGN)) {
            checkout(pskip(parser));
            checkout(par_value(parser));
            int id = sumka_refl_lup_id(parser->refl, strmk(parser, &name));
            
            if (id == -1)
                return SUMKA_ERR_VARIABLE_NOT_FOUND;

            SumkaReflItem *item = parser->refl->stack[id];

            if (!sumka_refl_instanceof(item, parser->last_item.type))
                return SUMKA_ERR_TYPE_MISMATCH;

            int relative = id - parser->stack_base; 

            sumka_codegen_instr_iuc(&parser->cg, SUMKA_INSTR_SET, relative);
        }
        else if (pif(parser, SUMKA_TT_DEFN)) {
            checkout(par_defn(name, parser));
        }
        else {
            *parser = prev_parser;
            *parser->lexer = lexer;
            checkout(par_value(parser));
        }
    }
    return SUMKA_OK;
}

static
SumkaError par_body(SumkaParser *parser) {
    size_t previous_base = parser->refl->stack_size;
    checkout(pthen(parser, SUMKA_TT_LBRACE));
    while (pifnt(parser, SUMKA_TT_RBRACE))
        checkout(decide_statement(parser));  
    checkout(pthen(parser, SUMKA_TT_RBRACE));
    
    while (parser->refl->stack_size > previous_base) {
        sumka_codegen_instr_iuc(&parser->cg, SUMKA_INSTR_SET, parser->refl->stack_size - parser->stack_base - 1);
        parser->refl->stack_size -= 1;
    }
    return SUMKA_OK;
}

static
SumkaError par_fn(SumkaParser *parser) {
    checkout(pthen(parser, SUMKA_TT_KW_FN));
    SumkaToken name;
    checkout(pgive(parser, SUMKA_TT_IDENT, &name));
    
    parser->stack_base = parser->refl->stack_size+1;
 
    SumkaReflItem *item = sumka_refl_lup(parser->refl, strmk(parser, &name));
    
    if (item && item->tag != SUMKA_TAG_FUN)
        item = NULL;

    // If item isn't null then there was a declaration before
    // FIXME: This will re-define declarations, unfortunately
    bool implementation = item != NULL;
    
    if (item == NULL)
        item = sumka_refl_make_fn(parser->refl, strmk(parser, &name), parser->cg.instr_count);
    else 
        // Reset address for the definition
        item->fn.addr = parser->cg.instr_count;

    sumka_refl_push(parser->refl, item);
    size_t current_frame = sumka_refl_peek(parser->refl);
    checkout(pthen(parser, SUMKA_TT_LPAREN));

    SumkaReflItem *arg = item->fn.first_arg;

    int argc = 0;

    while (pifnt(parser, SUMKA_TT_RPAREN)) {
        argc += 1;
        if (implementation && arg == NULL)
            return SUMKA_ERR_TOO_MANY_ARGS;

        SumkaToken param_name;
        checkout(pgive(parser, SUMKA_TT_IDENT, &param_name));
        checkout(pthen(parser, SUMKA_TT_COLON));

        SumkaReflItem *param_type;
        checkout(par_type(parser, &param_type));
        assert(param_type != NULL);
        SumkaReflItem *param = sumka_refl_make_var(parser->refl, strmk(parser, &param_name), param_type);
        sumka_refl_push(parser->refl, param);

        if (!implementation)
            sumka_refl_add_param(item, param);
        else if (!sumka_refl_instanceof(arg, param_type))
            return SUMKA_ERR_TYPE_MISMATCH;
        else 
            arg = arg->sibling;
        
        if (pif(parser, SUMKA_TT_RPAREN))
            break;
        
        checkout(pthen(parser, SUMKA_TT_COMMA));
    }

    if (implementation && arg != NULL)
        return SUMKA_ERR_NOT_ENOUGH_ARGS;

    checkout(pthen(parser, SUMKA_TT_RPAREN));

    if (pif(parser, SUMKA_TT_COLON)) {
        checkout(pskip(parser));
        checkout(par_type(parser, &item->type)); 
    }
    else {
        item->type = sumka_refl_find(parser->refl, "void");
    }
    parser->last_item = sumka_refl_make_dummy(item->type, SUMKA_TAG_FUN);
    parser->return_type = item->type;

    // Is definition or forward decl. 
    if (pif(parser, SUMKA_TT_LBRACE)) {
        // Borrowing nothing is redundant, hence the check
        if (argc != 0)
            sumka_codegen_instr_iuc(&parser->cg, SUMKA_INSTR_BORROW_IUC, argc);
        checkout(par_body(parser));
        if (parser->return_type != sumka_refl_find(parser->refl, "void"))
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
    
    sumka_refl_seek(parser->refl, current_frame);
    return SUMKA_OK;
}

SumkaError sumka_parser_parse(SumkaParser *parser) {
    checkout(sumka_lex_next(parser->lexer, &parser->current_));
    while(!parser->eof) checkout(par_fn(parser));
    return SUMKA_OK;
}


