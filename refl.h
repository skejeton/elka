/* Reflection for Elka
 * Copyright (C) 2021 ishdx2
 * Licensed under MIT license
 */
#ifndef ELKA_REFL_H__
#define ELKA_REFL_H__

#include <stddef.h>
#include <stdbool.h>

struct ElkaRuntime;
typedef void (*ElkaFFIExec)(struct SumkaRuntime *runtime);

typedef enum ElkaReflItemKind {
    ELKA_TAG_FFIFUN,
    ELKA_TAG_FUN,
    ELKA_TAG_VALUE,
    ELKA_TAG_BASETYPE,
    ELKA_TAG_ARRAY,
    ELKA_TAG_INTRIN
} ElkaReflItemKind;

typedef enum ElkaBasetype {
    ELKA_TYPE_VOID,
    ELKA_TYPE_STR,
    ELKA_TYPE_INT,
} ElkaBasetype;

typedef struct ElkaReflItem {
    char *name;
    
    // NOTE: For functions this is the return type
    enum ElkaReflItemKind tag;
    struct ElkaReflItem *type; 
    bool present;
    union {
        struct {
            size_t addr;
            struct ElkaReflItem *first_arg;
        } fn;
        ElkaBasetype basetype;
        ElkaFFIExec ffi_fn;
        size_t array_size;
    };
    struct ElkaReflItem *sibling;   
} ElkaReflItem;

typedef struct ElkaRefl {
    ElkaReflItem refls[2048];
    size_t refl_count;
    ElkaReflItem *stack[2048];
    size_t stack_size;
} ElkaRefl;

/// @returns Refection item of a base type
const ElkaReflItem *elka_basetype(SumkaReflItemKind kind);

// NOTE: This implicitly adds the FFI function onto the reflection stack
//       For convenience
void elka_refl_register_ffi_fn(ElkaRefl *refl, char *name, SumkaFFIExec exec);

ElkaReflItem *elka_refl_make_fn(SumkaRefl *refl, char *name, size_t addr);

ElkaReflItem *elka_refl_make_var(SumkaRefl *refl, char *name, SumkaReflItem *type);

/// @brief Returns a dummy reflection item that simulates a value/instance
ElkaReflItem elka_refl_make_dummy(SumkaReflItem *type, SumkaReflItemKind kind);

/// @returns if `value` is an instance of type `type`
bool elka_refl_instanceof(ElkaReflItem *value, SumkaReflItem *type);

/// @brief Debug tracing function 
void elka_refl_trace(ElkaRefl *refl);

ElkaReflItem *elka_refl_find(SumkaRefl *refl, const char *name);

void elka_refl_add_param(ElkaReflItem *item, SumkaReflItem *other);

/// @brief This will look up reflection item on the stack, this is the
///        recommended function to use when you want to look something up
ElkaReflItem *elka_refl_lup(SumkaRefl *refl, const char *name);

/// @brief This does the same as the function above, but instead returns an index
///        For the reflection item, this is probably [temporary]
///        NOTE: This returns an index into the REFLECTION STACK!!
int elka_refl_lup_id(ElkaRefl *refl, const char *name);

size_t elka_refl_peek(ElkaRefl *refl);

void elka_refl_seek(ElkaRefl *refl, size_t n);

void elka_refl_push(ElkaRefl *refl, SumkaReflItem *item);

void elka_refl_dispose(ElkaRefl *refl);

// @returns A new reflection with basetypes initialized
ElkaRefl elka_refl_new();

#endif // ELKA_REFL_H__
