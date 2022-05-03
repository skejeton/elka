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

void print(ElkaRuntime *rt) {
    printf("%s", elka_runtime_pop_str(rt)); 
}

void printi(ElkaRuntime *rt) {
    //printf("%i", elka_runtime_pop_int(rt)); 
}

void println(ElkaRuntime *rt) {
    (void) rt;
    printf("\n"); 
}

int main() {
    char *source = read_file("playground/main.um");
    ElkaLexer lexer = { .source = source };
    
    ElkaRefl refl = elka_refl_new();    

    elka_refl_register_ffi_fn(&refl, "print", print);
    elka_refl_register_ffi_fn(&refl, "printi", printi);
    elka_refl_register_ffi_fn(&refl, "println", println);
    
    
    ElkaParser parser = { .lexer = &lexer, .refl = &refl };
    parser.cg.refl = &refl;
    /*
    ElkaToken tok;
    ElkaError err;
    while ((err = elka_lex_next(&lexer, &tok)) == ELKA_OK) {
        elka_token_dbgdmp(&lexer, &tok);
    }

    if (err && err != ELKA_ERR_EOF) {
        printf("Error number %d, %d:%d\n", err, lexer.line_+1, lexer.column_+1);
        return -1;
    }
    */
    
    ElkaError err = elka_parser_parse(&parser);
    elka_refl_trace(parser.refl);
    if (err) {
        elka_refl_dispose(parser.refl);
        printf("Error at %d:%d\n", parser.current_.line+1, parser.current_.column+1);
        //elka_parser_print_error(&parser, err);
        free(source);
        return -1;
    }

    printf(" == Bytecode == \n");
    elka_codegen_dbgdmp(&parser.cg);
    fflush(stdout);
    printf(" == Execution == \n");
    ElkaRuntime runtime = { .cg = &parser.cg };

    elka_runtime_exec(&runtime);
    elka_refl_dispose(&refl);
    elka_codegen_dispose(&parser.cg);
    free(source);
}
