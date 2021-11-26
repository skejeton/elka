#ifndef SUMKA_GENERAL_H__
#define SUMKA_GENERAL_H__

typedef enum {
    SUMKA_OK,                       // 0
    SUMKA_ERR_PLACEHOLDER,          // 1
    SUMKA_ERR_VARIABLE_NOT_FOUND,   // 2
    SUMKA_ERR_FUNCTION_NOT_FOUND,   // 3
    SUMKA_ERR_TYPE_MISMATCH,        // 4
    SUMKA_ERR_EOF,                  // 5
    SUMKA_ERR_INVALID_TOKEN,        // 6
    SUMKA_ERR_INVALID_CHARACTER,    // 7 
    SUMKA_ERR_NUMBER_OUT_OF_RANGE,  // 8
    SUMKA_ERR_INVALID_TYPE,         // 9
    SUMKA_ERR_TYPE_NOT_FOUND,       // 10
    SUMKA_ERR_CALL_ON_NON_FUNCTION, // 11
    SUMKA_ERR_TOO_MANY_ARGS,        // 12
    SUMKA_ERR_NOT_ENOUGH_ARGS,      // 13
    SUMKA_ERR_DOUBLE_FORWARD_DECL   // 14
} __attribute__((warn_unused_result)) SumkaError;

#endif

