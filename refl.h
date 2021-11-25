/* Reflection for Sumka
 * Copyright (C) 2021 ishdx2
 * Licensed under MIT license
 */

#include <stddef.h>
#include <stdbool.h>

enum SumkaTypeKind {
    SUMKA_TYPE_VOID,
    // TODO: Remove this, instead make ReflItems for string/int type, et cetera
    SUMKA_TYPE_STR,
    SUMKA_TYPE_INT,
    SUMKA_TYPE_FUN,
    SUMKA_TYPE_TYPE
};

typedef struct SumkaReflItem {
    char *name;
    enum SumkaTypeKind type;
    bool present;
    union {
        struct {
            size_t addr;
            struct SumkaReflItem *ret_type;
            struct SumkaReflItem *first_arg;
        } fn;
    };
    struct SumkaReflItem *sibling;   
} SumkaReflItem;

typedef struct SumkaRefl {
    SumkaReflItem refls[2048];
    size_t refl_count;
    SumkaReflItem *stack[2048];
    size_t stack_size;
} SumkaRefl;

SumkaReflItem *sumka_refl_make_fn(SumkaRefl *refl, char *name, size_t addr);
SumkaReflItem *sumka_refl_make_var(SumkaRefl *refl, char *name, enum SumkaTypeKind type);
bool sumka_refl_typecheck(SumkaReflItem *against, enum SumkaTypeKind type);
void sumka_refl_trace(SumkaRefl *refl);
int sumka_refl_find(SumkaRefl *refl, const char *name);
void sumka_refl_add_param(SumkaReflItem *item, SumkaReflItem *other);
int sumka_refl_lup(SumkaRefl *refl, const char *name);
size_t sumka_refl_peek(SumkaRefl *refl);
void sumka_refl_seek(SumkaRefl *refl, size_t n);
void sumka_refl_push(SumkaRefl *refl, SumkaReflItem *item);
void sumka_refl_dispose(SumkaRefl *refl);
