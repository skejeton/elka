#include "ast.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>

#define CAPACITY (1 << 16)

// Creates an ast
ElkaAst elka_ast_init() {
	ElkaNode *nodes = malloc(CAPACITY*sizeof(Node));
	return (ElkaAst) { .nodes = nodes };
}

// Makes a zero initialized node
ElkaNode* elka_ast_make(Ast* ast, Node* parent) {
	// Allocate a new node
	ElkaNode *new_node = &ast->nodes[ast->node_count++];
	
	// Zero-initialize the node
	*new_node = (ElkaNode) { 0 };

	// Make node if we have a parent
	if (parent != NULL)
	{
	// Find previous item before last child
	Node **n;
	for (n = &parent->first_child; *n; n = &(*n)->sibling);
	
	// Attach the node
	*n = new_node;
	}
	
	return new_node;
}

// Safely assigns some fields from source, returns destination
ElkaNode* node_set(ElkaNode *destination, SumkaNode source) {
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


ElkaNode node_of(ElkaNodeType type) {
	return (ElkaNode) {
	.type = type,
	};
}


ElkaNode node_from(ElkaNodeType type, SumkaToken token) {
	return (ElkaNode) {
		.type = type,
		.token = token
	};
}

// Returns a child at index n, NULL if out of bounds
ElkaNode* node_child(ElkaNode *node, size_t n) {
	ElkaNode *child = node->first_child;
	while (n-- && child) {
		child = child->sibling;
	}
	return child;
}

// Frees ast data
void ast_deinit(Ast* ast) {
	free(ast->nodes);
}	

void node_pretty_print(ElkaNode *node, int indent) {
	printf("%*c\x1b[34m", indent, ' ');

	const char *types[] = {"ident", "fn", "colon", "left paren", "right paren",
		"left brace", "right brace", "string", "number", "defn", "comma",
		"return", "if", "assign", "for", "left bracket", "right bracket"}

	printf("%s: \x1b[37m", types[node->token.type]);
	
	// Invalid token usually means there's no need for a token
	// So we are just not gonna print them
	if (node->token.type != tok_inval) {
		print_tok(node->token);
	}
	else {
		printf("\n");
	}
	
	printf("\x1b[0m");

	if (node->first_child) {
		// printf("%*c(\n", indent, ' ');
		node_pretty_print(node->first_child, indent + 2);
		// printf("%*c)\n", indent, ' ');
	}

	if (node->sibling)
		node_pretty_print(node->sibling, indent);
}

void ast_pretty_print(ElkaAst *ast) {
	node_pretty_print(ast->nodes, 1);
}

