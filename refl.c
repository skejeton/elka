#include "refl.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct {
    const char *name;
    ElkaBasetype basetype;
} BASETYPE_REFL[] = {
    [ELKA_TYPE_VOID] = { "void", SUMKA_TYPE_VOID },
    [ELKA_TYPE_INT]  = { "int", SUMKA_TYPE_INT },
    [ELKA_TYPE_STR]  = { "str", SUMKA_TYPE_STR },
};

static
ElkaReflItem generic_item(const char *name, SumkaReflItemKind tag) {
    return (ElkaReflItem) {
        // FIXME: Remove strdup here
        // NOTE: If we recieve an empty string we don't strdup it, this is for dummy maker thingey
        .name = *name ? strdup(name) : "",
        .present = true,
        .tag = tag
    };   
}

void elka_refl_register_ffi_fn(ElkaRefl *refl, char *name, SumkaFFIExec exec) {
    ElkaReflItem *item = &refl->refls[refl->refl_count++];

    *item = generic_item(name, ELKA_TAG_FFIFUN);
    item->ffi_fn = exec;
    
    elka_refl_push(refl, item);
}

ElkaReflItem elka_refl_make_dummy(SumkaReflItem *type, SumkaReflItemKind kind) {
    ElkaReflItem item = generic_item("", kind);
    item.type = type;
    return item;
}

ElkaReflItem *elka_refl_make_fn(SumkaRefl *refl, char *name, size_t addr) {
    ElkaReflItem *item = &refl->refls[refl->refl_count++];

    *item = generic_item(name, ELKA_TAG_FUN);
    item->fn.addr = addr;
    
    return item;
}

ElkaReflItem *mkbasetype(SumkaRefl *refl, const char *name, SumkaBasetype basetype) {
    ElkaReflItem *item = &refl->refls[refl->refl_count++];
    *item = generic_item(name, ELKA_TAG_BASETYPE);
    item->basetype = basetype;
    return item;
}

ElkaRefl elka_refl_new() {
    ElkaRefl result = { 0 };
    const int count = sizeof(BASETYPE_REFL)/sizeof(BASETYPE_REFL[0]);
    // NOTE: This will copy the basetype definitions
    //       This is necessary because they are const
    //       But refl_find/lup require non-const to be returned     
    for (int i = 0; i < count; i += 1) {
        ElkaReflItem *item = mkbasetype(&result, BASETYPE_REFL[i].name, BASETYPE_REFL[i].basetype);
        elka_refl_push(&result, item);
    }    
    return result;
}

ElkaReflItem *elka_refl_find(SumkaRefl *refl, const char *name) {
    for (size_t i = 0; i < refl->refl_count; i += 1) {
        if (strcmp(refl->refls[i].name, name) == 0)
            return &refl->refls[i];
    }
    return NULL;
}

int elka_refl_lup_id(ElkaRefl *refl, const char *name) {
    for (int i = refl->stack_size-1; i >= 0; i -= 1) {
        if (strcmp(refl->stack[i]->name, name) == 0)
            return i;
    }
    return -1;
}

ElkaReflItem *elka_refl_lup(SumkaRefl *refl, const char *name) {
    int id = elka_refl_lup_id(refl, name);
    return id == -1 ? NULL : refl->stack[id];
}


bool elka_refl_instanceof(ElkaReflItem *a, SumkaReflItem *b) {
    assert(a != NULL);
    // Note: This only works for now as we don't have type aliases
    // When this will be added we can't just check pointers
    return a->type == b;
}

ElkaReflItem *elka_refl_make_var(SumkaRefl *refl, char *name, SumkaReflItem *type) {
    ElkaReflItem *item = &refl->refls[refl->refl_count++];
    *item = generic_item(name, ELKA_TAG_VALUE);
    item->type = type;
    return item;
}

void elka_refl_trace(ElkaRefl *refl) {
    const char *types[] = {
        [ELKA_TAG_FFIFUN]   = "FFIFunction",
        [ELKA_TAG_FUN]      = "Function",
        [ELKA_TAG_VALUE]    = "Value",
        [ELKA_TAG_BASETYPE] = "Basetype",
        [ELKA_TAG_ARRAY]    = "Array",
        [ELKA_TAG_INTRIN]   = "Intrinsic",
    };
    
    for (int i = refl->stack_size-1; i >= 0; i -= 1) {

        printf(":: %s %s %d\n", refl->stack[i]->name,
                                types[refl->stack[i]->tag],
                                refl->stack[i]->present);
    }
}

void elka_refl_add_param(ElkaReflItem *item, SumkaReflItem *other) {

    assert(item->tag == ELKA_TAG_FUN && "Can't add parameter to non function");

    if (item->fn.first_arg) {
        ElkaReflItem *i = item->fn.first_arg;
        
        while(i->sibling) {
           i = i->sibling;
        }

        i->sibling = other;
    }
    else {
        item->fn.first_arg = other;
    }
}

size_t elka_refl_peek(ElkaRefl *refl) {
    return refl->stack_size;
}

void elka_refl_seek(ElkaRefl *refl, size_t n) {
    refl->stack_size = n;
}

void elka_refl_push(ElkaRefl *refl, SumkaReflItem *item) {
    refl->stack[refl->stack_size++] = item;
}

void elka_refl_dispose(ElkaRefl *refl) {
    for (size_t i = 0; i < refl->refl_count; i += 1) 
        if (refl->refls[i].name)
            free(refl->refls[i].name);
}
