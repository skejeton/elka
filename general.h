#ifndef SUMKA_GENERAL_H__
#define SUMKA_GENERAL_H__

typedef enum {
    SUMKA_OK,
    SUMKA_ERR_PLACEHOLDER,
    SUMKA_ERR_EOF,
    SUMKA_ERR_INVALID_TOKEN,
    SUMKA_ERR_INVALID_CHARACTER,
    SUMKA_ERR_NUMBER_OUT_OF_RANGE,
    SUMKA_ERR_UNKNOWN_TYPE
} __attribute__((warn_unused_result)) SumkaError;

#endif

