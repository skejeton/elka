#include "refl.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Basetype definitions */
static const SumkaReflItem BASETYPE_REFL[] = {
    [SUMKA_TYPE_VOID] = {
        .tag      = SUMKA_TAG_BASETYPE,
        .name     = "void",
        .present  = true,
        .basetype = SUMKA_TYPE_VOID,
        .sibling  = NULL, 
    },
    [SUMKA_TYPE_INT] = {
        .tag      = SUMKA_TAG_BASETYPE,
        .name     = "int",
        .present  = true,
        .basetype = SUMKA_TYPE_INT,
        .sibling  = NULL, 
    },
    [SUMKA_TYPE_STR] = {
        .tag      = SUMKA_TAG_BASETYPE,
        .name     = "str",
        .present  = true,
        .basetype = SUMKA_TYPE_STR,
        .sibling  = NULL, 
    },
};

const SumkaReflItem *sumka_basetype(SumkaReflItemKind kind) {
    return &BASETYPE_REFL[kind];
}

SumkaReflItem *sumka_refl_make_fn(SumkaRefl *refl, char *name, size_t addr) {
    SumkaReflItem *item = &refl->refls[refl->refl_count++];

    *item = (SumkaReflItem) { 0 };

    item->present = true;
    // FIXME: Remove strdup here
    item->name = strdup(name);
    item->fn.addr = addr;
    return item;
}

// This will find built-in reflection items
static
const SumkaReflItem *find_basic(SumkaRefl *refl, const char *name) {
    for (int i = 0; i < sizeof(BASETYPE_REFL)/sizeof(SumkaReflItem); i += 1) {
        if (strcmp(BASETYPE_REFL[i].name, name) == 0) {
            return &BASETYPE_REFL[i];
        }
    }
    return NULL;
}

const
SumkaReflItem *sumka_refl_find(SumkaRefl *refl, const char *name) {
    const SumkaReflItem *item = find_basic(refl, name);
    if (item != NULL) return item;
    for (size_t i = 0; i < refl->refl_count; i += 1) {
        if (strcmp(refl->refls[i].name, name) == 0)
            return &refl->refls[i];
    }
    return NULL;
}

const
SumkaReflItem *sumka_refl_lup(SumkaRefl *refl, const char *name) {
    const SumkaReflItem *item = find_basic(refl, name);
    if (item != NULL) return item;
    for (int i = refl->stack_size-1; i >= 0; i -= 1) {
        if (strcmp(refl->stack[i]->name, name) == 0)
            return &refl->refls[i];
    }
    return NULL;
}

bool sumka_refl_instanceof(SumkaReflItem *a, SumkaReflItem *b) {
    // Note: This only works for now as we don't have type aliases
    // When this will be added we can't just check pointers
    return b->type == a;
}

SumkaReflItem *sumka_refl_make_var(SumkaRefl *refl, char *name, SumkaReflItem *type) {
    SumkaReflItem *item = &refl->refls[refl->refl_count++];

    // FIXME: Remove strdup here
    item->name = strdup(name);
    item->type = type;
    return item;
}



void sumka_refl_trace(SumkaRefl *refl) {
    const char *types[] = {
        [SUMKA_TAG_FUN]      = "Function",
        [SUMKA_TAG_VALUE]    = "Value",
        [SUMKA_TAG_BASETYPE] = "Basetype"
    };
    
    for (int i = refl->stack_size-1; i >= 0; i -= 1) {
        printf(":: %s %s %d\n", refl->stack[i]->name, types[refl->stack[i]->tag], refl->stack[i]->present);
    }
}

void sumka_refl_add_param(SumkaReflItem *item, SumkaReflItem *other) {
    assert(item->tag == SUMKA_TAG_FUN && "Can't add parameter to non function");

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
