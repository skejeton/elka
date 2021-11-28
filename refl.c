#include "refl.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct {
    const char *name;
    SumkaBasetype basetype;
} BASETYPE_REFL[] = {
    [SUMKA_TYPE_VOID] = { "void", SUMKA_TYPE_VOID },
    [SUMKA_TYPE_INT]  = { "int", SUMKA_TYPE_INT },
    [SUMKA_TYPE_STR]  = { "str", SUMKA_TYPE_STR },
};

static
SumkaReflItem generic_item(const char *name, SumkaReflItemKind tag) {
    return (SumkaReflItem) {
        // FIXME: Remove strdup here
        .name = strdup(name),
        .present = true,
        .tag = tag
    };   
}

void sumka_refl_register_ffi_fn(SumkaRefl *refl, char *name, SumkaFFIExec exec) {
    SumkaReflItem *item = &refl->refls[refl->refl_count++];

    *item = generic_item(name, SUMKA_TAG_FFIFUN);
    item->ffi_fn = exec;
    
    sumka_refl_push(refl, item);
}

SumkaReflItem *sumka_refl_make_fn(SumkaRefl *refl, char *name, size_t addr) {
    SumkaReflItem *item = &refl->refls[refl->refl_count++];

    *item = generic_item(name, SUMKA_TAG_FUN);
    item->fn.addr = addr;
    
    return item;
}

SumkaReflItem *mkbasetype(SumkaRefl *refl, const char *name, SumkaBasetype basetype) {
    SumkaReflItem *item = &refl->refls[refl->refl_count++];
    *item = generic_item(name, SUMKA_TAG_BASETYPE);
    item->basetype = basetype;
    return item;
}

SumkaRefl sumka_refl_new() {
    SumkaRefl result = { 0 };
    const int count = sizeof(BASETYPE_REFL)/sizeof(BASETYPE_REFL[0]);
    // NOTE: This will copy the basetype definitions
    //       This is necessary because they are const
    //       But refl_find/lup require non-const to be returned     
    for (int i = 0; i < count; i += 1) {
        SumkaReflItem *item = mkbasetype(&result, BASETYPE_REFL[i].name, BASETYPE_REFL[i].basetype);
        sumka_refl_push(&result, item);
    }    
    return result;
}

SumkaReflItem *sumka_refl_find(SumkaRefl *refl, const char *name) {
    for (size_t i = 0; i < refl->refl_count; i += 1) {
        if (strcmp(refl->refls[i].name, name) == 0)
            return &refl->refls[i];
    }
    return NULL;
}

int sumka_refl_lup_id(SumkaRefl *refl, const char *name) {
    for (int i = refl->stack_size-1; i >= 0; i -= 1) {
        if (strcmp(refl->stack[i]->name, name) == 0)
            return i;
    }
    return -1;
}

SumkaReflItem *sumka_refl_lup(SumkaRefl *refl, const char *name) {
    int id = sumka_refl_lup_id(refl, name);
    return id == -1 ? NULL : refl->stack[id];
}


bool sumka_refl_instanceof(SumkaReflItem *a, SumkaReflItem *b) {
    assert(a != NULL);
    // Note: This only works for now as we don't have type aliases
    // When this will be added we can't just check pointers
    return a->type == b;
}

SumkaReflItem *sumka_refl_make_var(SumkaRefl *refl, char *name, SumkaReflItem *type) {
    SumkaReflItem *item = &refl->refls[refl->refl_count++];
    *item = generic_item(name, SUMKA_TAG_VALUE);
    item->type = type;
    return item;
}

void sumka_refl_trace(SumkaRefl *refl) {
    const char *types[] = {
        [SUMKA_TAG_FFIFUN]   = "FFIFunction",
        [SUMKA_TAG_FUN]      = "Function",
        [SUMKA_TAG_VALUE]    = "Value",
        [SUMKA_TAG_BASETYPE] = "Basetype"
    };
    
    for (int i = refl->stack_size-1; i >= 0; i -= 1) {

        printf(":: %s %s %d\n", refl->stack[i]->name,
                                types[refl->stack[i]->tag],
                                refl->stack[i]->present);
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
