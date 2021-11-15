/* Type system for Sumka
 * Copyright (C) 2021 ishdx2
 * Licensed under MIT license
 */

#ifndef SUMKA_TYPESYSTEM_H__
#define SUMKA_TYPESYSTEM_H__

#include <stdbool.h>

typedef struct SumkaType {
    enum SumkaTypeKind {
        SUMKA_TYPE_STR,
        SUMKA_TYPE_INT,
    } kind;
} SumkaType;

bool sumka_types_compatible(SumkaType a, SumkaType b);
#endif
