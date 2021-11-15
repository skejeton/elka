#include "typesystem.h"

bool sumka_types_compatible(SumkaType a, SumkaType b) {
    return a.kind == b.kind;
}
