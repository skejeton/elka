/* Reflection for Sumka
 * Copyright (C) 2021 ishdx2
 * Licensed under MIT license
 */

#include <stddef.h>
#include <stdbool.h>
#include "ffi.h"

typedef enum SumkaReflItemKind {
    SUMKA_TAG_FUN,
    SUMKA_TAG_VALUE,
    SUMKA_TAG_BASETYPE
} SumkaReflItemKind;

typedef enum SumkaBasetype {
    SUMKA_TYPE_VOID,
    SUMKA_TYPE_STR,
    SUMKA_TYPE_INT,
} SumkaBasetype;

typedef struct SumkaReflItem {
    char *name;
    
    // NOTE: For functions this is the return type
    enum SumkaReflItemKind tag;
    struct SumkaReflItem *type; 
    bool present;
    union {
        struct {
            size_t addr;
            struct SumkaReflItem *first_arg;
        } fn;
        SumkaBasetype basetype;
    };
    struct SumkaReflItem *sibling;   
} SumkaReflItem;

typedef struct SumkaRefl {
    SumkaFFI *ffi;
    SumkaReflItem refls[2048];
    size_t refl_count;
    SumkaReflItem *stack[2048];
    size_t stack_size;
} SumkaRefl;

/// @returns Refection item of a base type
const SumkaReflItem *sumka_basetype(SumkaReflItemKind kind);

SumkaReflItem *sumka_refl_make_fn(SumkaRefl *refl, char *name, size_t addr);

SumkaReflItem *sumka_refl_make_var(SumkaRefl *refl, char *name, SumkaReflItem *type);

bool           sumka_refl_typecheck(SumkaReflItem *against, SumkaReflItem *type);

void           sumka_refl_trace(SumkaRefl *refl);

const 
SumkaReflItem *sumka_refl_find(SumkaRefl *refl, const char *name);

void           sumka_refl_add_param(SumkaReflItem *item, SumkaReflItem *other);

const 
SumkaReflItem *sumka_refl_lup(SumkaRefl *refl, const char *name);

size_t         sumka_refl_peek(SumkaRefl *refl);

void           sumka_refl_seek(SumkaRefl *refl, size_t n);

void           sumka_refl_push(SumkaRefl *refl, SumkaReflItem *item);

void           sumka_refl_dispose(SumkaRefl *refl);
