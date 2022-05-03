#include "ast.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>

#define CAPACITY (1 << 16)

// Creates an ast
ElkaAst elka_ast_init() {
	ElkaNode *nodes = malloc(CAPACITY*sizeof(ElkaNode));
	return (ElkaAst) { .nodes = nodes };
}

// Makes a zero initialized node
ElkaNode* elka_ast_make(ElkaAst* ast, ElkaNode* parent) {
	// Allocate a new node
	ElkaNode *new_node = &ast->nodes[ast->node_count++];
	
	// Zero-initialize the node
	*new_node = (ElkaNode) { 0 };

	// Make node if we have a parent
	if (parent != NULL)
	{
	// Find previous item before last child
	ElkaNode **n;
	for (n = &parent->first_child; *n; n = &(*n)->sibling);
	
	// Attach the node
	*n = new_node;
	}
	
	return new_node;
}

// Safely assigns some fields from source, returns destination
ElkaNode* elka_node_set(ElkaNode *destination, ElkaNode source) {
	// Acquire sensetive fields
	ElkaNode *first_child = destination->first_child;
	ElkaNode *sibling = destination->sibling;

	*destination = source;
	
	// Set them back
	destination->first_child = first_child;
	destination->sibling = sibling;
	
	// Return same node
	return destination;
}


ElkaNode elka_node_of(ElkaNodeType type) {
	return (ElkaNode) {
	.type = type,
	};
}


ElkaNode elka_node_from(ElkaNodeType type, ElkaToken token) {
	return (ElkaNode) {
		.type = type,
		.token = token
	};
}

// Returns a child at index n, NULL if out of bounds
ElkaNode* elka_node_child(ElkaNode *node, size_t n) {
	ElkaNode *child = node->first_child;
	while (n-- && child) {
		child = child->sibling;
	}
	return child;
}

// Frees ast data
void elka_ast_deinit(ElkaAst* ast) {
	free(ast->nodes);
}	

void node_pretty_print(ElkaLexer *l, ElkaNode *node, int indent) {
	printf("%*c\x1b[34m", indent, ' ');

	const char *types[] = {"ident", "fn", "colon", "left paren", "right paren",
		"left brace", "right brace", "string", "number", "defn", "comma",
		"return", "if", "assign", "for", "left bracket", "right bracket",
		"asterisk", "caret", "EOF"};

	printf("%s: \x1b[37m", types[node->token.type]);
	
	elka_token_dbgdmp(l, &node->token);
	
	printf("\x1b[0m");

	if (node->first_child) {
		// printf("%*c(\n", indent, ' ');
		node_pretty_print(l, node->first_child, indent + 2);
		// printf("%*c)\n", indent, ' ');
	}

	if (node->sibling)
		node_pretty_print(l, node->sibling, indent);
}

void elka_ast_pretty_print(ElkaLexer *l, ElkaAst *ast) {
	node_pretty_print(l, ast->nodes, 1);
}

