/* Reflection for Sumka
 * Copyright (C) 2021 ishdx2
 * Licensed under MIT license
 */

#include <stddef.h>
#include <stdbool.h>

struct SumkaRuntime;
typedef void (*SumkaFFIExec)(struct SumkaRuntime *runtime);

typedef enum SumkaReflItemKind {
    SUMKA_TAG_FFIFUN,
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
        SumkaFFIExec ffi_fn;
    };
    struct SumkaReflItem *sibling;   
} SumkaReflItem;

typedef struct SumkaRefl {
    SumkaReflItem refls[2048];
    size_t refl_count;
    SumkaReflItem *stack[2048];
    size_t stack_size;
} SumkaRefl;

/// @returns Refection item of a base type
const SumkaReflItem *sumka_basetype(SumkaReflItemKind kind);

// NOTE: This implicitly adds the FFI function onto the reflection stack
//       For convenience
void           sumka_refl_register_ffi_fn(SumkaRefl *refl, char *name, SumkaFFIExec exec);

SumkaReflItem *sumka_refl_make_fn(SumkaRefl *refl, char *name, size_t addr);

SumkaReflItem *sumka_refl_make_var(SumkaRefl *refl, char *name, SumkaReflItem *type);

/// @returns if `value` is an instance of type `type`
bool           sumka_refl_instanceof(SumkaReflItem *value, SumkaReflItem *type);

/// @brief Debug tracing function 
void           sumka_refl_trace(SumkaRefl *refl);

SumkaReflItem *sumka_refl_find(SumkaRefl *refl, const char *name);

void           sumka_refl_add_param(SumkaReflItem *item, SumkaReflItem *other);

// @brief This will look up reflection item on the stack, this is the
//        recommended function to use when you want to look something up
SumkaReflItem *sumka_refl_lup(SumkaRefl *refl, const char *name);

// @brief This does the same as the function above, but instead returns an index
//        For the reflection item, this is probably [temporary]
//        NOTE: This returns an index into the REFLECTION STACK!!
int            sumka_refl_lup_id(SumkaRefl *refl, const char *name);

size_t         sumka_refl_peek(SumkaRefl *refl);

void           sumka_refl_seek(SumkaRefl *refl, size_t n);

void           sumka_refl_push(SumkaRefl *refl, SumkaReflItem *item);

void           sumka_refl_dispose(SumkaRefl *refl);

// @returns A new reflection with basetypes initialized
SumkaRefl sumka_refl_new();

