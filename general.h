#ifndef ELKA_GENERAL_H__
#define ELKA_GENERAL_H__

typedef enum {
    ELKA_OK,                       // 0
    ELKA_ERR_PLACEHOLDER,          // 1
    ELKA_ERR_VARIABLE_NOT_FOUND,   // 2
    ELKA_ERR_FUNCTION_NOT_FOUND,   // 3
    ELKA_ERR_TYPE_MISMATCH,        // 4
    ELKA_ERR_EOF,                  // 5
    ELKA_ERR_INVALID_TOKEN,        // 6
    ELKA_ERR_INVALID_CHARACTER,    // 7 
    ELKA_ERR_NUMBER_OUT_OF_RANGE,  // 8
    ELKA_ERR_INVALID_TYPE,         // 9
    ELKA_ERR_TYPE_NOT_FOUND,       // 10
    ELKA_ERR_CALL_ON_NON_FUNCTION, // 11
    ELKA_ERR_TOO_MANY_ARGS,        // 12
    ELKA_ERR_NOT_ENOUGH_ARGS,      // 13
    ELKA_ERR_DOUBLE_FORWARD_DECL,  // 14
    ELKA_ERR_LITERAL_EXCESS_ITEMS, // 15
    ELKA_ERR_LITERAL_FEW_ITEMS,    // 16
    ELKA_ERR_NEAGTIVE_SIZED_ARRAY  // 17
} __attribute__((warn_unused_result)) ElkaError;

#endif

