#include <stdio.h>

#include "block_api.h"

void print_node(struct Node *node)
{
	printf("In node %d exist %d keys and values\n", node->num_vertix, node->n);
	fflush(stdout);
	if (node->leaf) {
		printf("This node is also a leaf\n");
		fflush(stdout);
	}
	else {
		printf("This node is not a leaf\n");
		fflush(stdout);
	}
	int i;
	for (i = 0; i < node->n; i++) {
		printf("Size of key is %d and value is %s\n", node->keys[i].size, 
			(char *)node->keys[i].data);
		fflush(stdout);
		printf("Size of value is %d and value is %s\n", node->values[i].size, 
			(char *)node->values[i].data);
		fflush(stdout);
	}

	if (!node->leaf) {
		for (i = 0; i < node->n+1; i++) {
			printf("Child %d is %d\n", i, node->children[i]);
			fflush(stdout);
		}
	}
	printf("\n");
	fflush(stdout);

	return;
}

void print_status(struct DB *db, struct Node *node)
{
	printf("In node %d exist %d keys and values\n", node->num_vertix, node->n);
	if (node->leaf)
		printf("This node is also a leaf\n");
	else
		printf("This node is not a leaf\n");
	int i;
	for (i = 0; i < node->n; i++) {
		printf("Size of key is %d and value is %s\n", node->keys[i].size, 
			(char *)node->keys[i].data);
		printf("Size of value is %d and value is %s\n", node->values[i].size, 
			(char *)node->values[i].data);
	}

	if (!node->leaf) {
		for (i = 0; i < node->n+1; i++) {
			printf("Child %d is %d\n", i, node->children[i]);
		}
	}
	printf("\n");
	fflush(stdout);

	if (!node->leaf) {
		for (i = 0; i < node->n+1; i++) {
			struct Node *new_node = db->open_node(db, node->children[i]);
			print_status(db, new_node);
			new_node->close_node(db, new_node);
			free(new_node);
		}
	}
	return;
}