#include "refl.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

SumkaReflItem *sumka_refl_make_fn(SumkaRefl *refl, char *name, size_t addr) {
    SumkaReflItem *item = &refl->refls[refl->refl_count++];

    *item = (SumkaReflItem) { 0 };

    item->present = true;
    // FIXME: Remove strdup here
    item->name = strdup(name);
    item->type = SUMKA_TYPE_FUN;
    item->fn.addr = addr;
    return item;
}


int sumka_refl_find(SumkaRefl *refl, const char *name) {
    for (size_t i = 0; i < refl->refl_count; i += 1) {
        if (strcmp(refl->refls[i].name, name) == 0)
            return i;
    }
    return -1;
}

int sumka_refl_lup(SumkaRefl *refl, const char *name) {
    for (int i = refl->stack_size-1; i >= 0; i -= 1) {
        if (strcmp(refl->stack[i]->name, name) == 0)
            return i;
    }
    return -1;
}

bool sumka_refl_typecheck(SumkaReflItem *against, enum SumkaTypeKind type) {
    return against->type == type;
}

SumkaReflItem *sumka_refl_make_var(SumkaRefl *refl, char *name, enum SumkaTypeKind type) {
    SumkaReflItem *item = &refl->refls[refl->refl_count++];

    // FIXME: Remove strdup here
    item->name = strdup(name);
    item->type = type;
    return item;
}



void sumka_refl_trace(SumkaRefl *refl) {
    const char *types[] = {
        [SUMKA_TYPE_INT] = "Integer",
        [SUMKA_TYPE_STR] = "String",
        [SUMKA_TYPE_FUN] = "Function"
    };
    
    for (int i = refl->stack_size-1; i >= 0; i -= 1) {
        printf(":: %s %s %d\n", refl->stack[i]->name, types[refl->stack[i]->type], refl->stack[i]->present);
    }
}

void sumka_refl_add_param(SumkaReflItem *item, SumkaReflItem *other) {
    assert(item->type == SUMKA_TYPE_FUN && "Can't add parameter to non function");

    if (item->fn.first_arg) {
        SumkaReflItem *i = item->fn.first_arg;
        
        while(i->sibling) {
           i = i->sibling;
        }

        i->sibling = other;
    }
    else {
        item->fn.first_arg = other;
    }
}

size_t sumka_refl_peek(SumkaRefl *refl) {
    return refl->stack_size;
}

void sumka_refl_seek(SumkaRefl *refl, size_t n) {
    refl->stack_size = n;
}

void sumka_refl_push(SumkaRefl *refl, SumkaReflItem *item) {
    refl->stack[refl->stack_size++] = item;
}

void sumka_refl_dispose(SumkaRefl *refl) {
    for (size_t i = 0; i < refl->refl_count; i += 1) 
        if (refl->refls[i].name)
            free(refl->refls[i].name);
}
