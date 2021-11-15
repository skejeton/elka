#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "runtime.h"

char* read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return NULL;
    fseek(f, 0, SEEK_END);
    size_t fsz = (size_t) ftell(f);
    fseek(f, 0, SEEK_SET);
    char *input = (char*)malloc((fsz+1)*sizeof(char));
    input[fsz] = 0;
    fread(input, sizeof(char), fsz, f);
    fclose(f);
    return input;
}

void print(SumkaRuntime *rt) {
    printf("%s\n", sumka_runtime_pop_str(rt)); 
}

void printi(SumkaRuntime *rt) {
    printf("%zi\n", sumka_runtime_pop_int(rt)); 
}

int main() {
    char *source = read_file("playground/main.um");
    SumkaLexer lexer = { .source = source };
    
    
    SumkaFFI ffi = { 0 };
    sumka_ffi_register(&ffi, "print", print);
    sumka_ffi_register(&ffi, "printi", printi);
    
    SumkaParser parser = { .lexer = &lexer, .ffi = &ffi };
    /*
    SumkaToken tok;
    SumkaError err;
    while ((err = sumka_lex_next(&lexer, &tok)) == SUMKA_OK) {
        sumka_token_dbgdmp(&lexer, &tok);
    }

    if (err && err != SUMKA_ERR_EOF) {
        printf("Error number %d, %d:%d\n", err, lexer.line_+1, lexer.column_+1);
        return -1;
    }
    */
    SumkaError err = sumka_parser_parse(&parser);
    if (err && err != SUMKA_ERR_EOF) {
        printf("Error number %d, %d:%d\n", err, parser.current_.line+1, parser.current_.column+1);
        return -1;
    }

    printf(" == Bytecode == \n");

    sumka_codegen_dbgdmp(&parser.cg);
    printf(" == Execution == \n");
    SumkaRuntime runtime = { .cg = &parser.cg, .ffi = &ffi };

    sumka_runtime_exec(&runtime);
    sumka_codegen_dispose(&parser.cg);
    free(source);
}
