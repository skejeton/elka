
#ifndef AST_H
#define AST_H

#include <stddef.h>
#include "lexer.h"


typedef enum ElkaNodeType {
} ElkaNodeType;


typedef struct ElkaNode {
	NodeType type;
	Token token;
	struct Node *first_child;
	struct Node *sibling;
} ElkaNode;


typedef struct ElkaAst {
	Node *nodes;
	size_t node_count;
} ElkaAst;

// Creates an ast
ElkaAst elka_ast_init();

// Makes a zero initialized node
ElkaNode* elka_ast_make(ElkaAst* ast, ElkaNode* parent);

// Returns a child at index n, NULL if out of bounds
ElkaNode* elka_node_child(ElkaNode *node, size_t n);

// Safely assigns some fields from source, returns destination
ElkaNode* elka_node_set(ElkaNode *destination, ElkaNode source);

// Convenient node constructor
ElkaNode elka_node_from(ElkaNodeType type, ElkaToken token);

// Constructs node from type
ElkaNode elka_node_of(ElkaNodeType type);

// Frees ast data
void elka_ast_deinit(ElkaAst* ast);

// Pretty prints node
void elka_node_pretty_print(ElkaNode *node, int indent);

// Pretty prints the ast
void elka_ast_pretty_print(ElkaAst *ast);

#endif // AST_H
