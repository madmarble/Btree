#include "block_api.h"
#include "third_party.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int new_memcmp(struct DBT t1, struct DBT t2)
{
	int res;
	if (t1.size > t2.size)
		res = 1;
	else if (t2.size > t1.size)
		res = -1;
	else
		res = memcmp(t1.data, t2.data, t1.size);
	return res;
}

int make_realloc(struct Node *node)
{
	node->keys = (struct DBT *)realloc(node->keys, node->n*sizeof(*node->keys));
	node->values = (struct DBT *)realloc(node->values, node->n*sizeof(*node->values));
	if (!node->leaf)
		node->children = (int *)realloc(node->children, (node->n+1)*sizeof(*node->children));
	return 0;
}

int delete_key(struct Node *node, int index)
{
	//printf("In delete_key\n");
	//fflush(stdout);
	free(node->keys[index].data);
	free(node->values[index].data);
	int i = index;
	while (i < node->n-1) {
		node->keys[i] = node->keys[i+1];
		node->values[i] = node->values[i+1];
		i++;
	}

	if (!node->leaf) {
		i = index+1;
		while (i < node->n) {
			node->children[i] = node->children[i+1];
			i++;
		}
	}
	node->n--;
	make_realloc(node);
	return 0;
}

int find_key(struct Node *node, struct DBT *key)
{
	int i = node->n-1;
	while(i >= 0 && new_memcmp(node->keys[i], *key)) {
		i--;
	}
	return i;
}

struct DBT deep_copy(struct DBT resource)
{
	struct DBT result;
	result.size = resource.size;
	result.data = calloc(result.size, 1);
	memcpy(result.data, resource.data, result.size);
	return result;
}

struct DBT * find_previous_key(struct DB *db, struct Node *node, struct DBT *key)
{
	if (node->leaf) {
		struct DBT *previous = (struct DBT *)malloc(2*sizeof(*previous));
		previous[0] = deep_copy(node->keys[node->n-1]);
		previous[1] = deep_copy(node->values[node->n-1]);
		return previous;
	}
	return find_previous_key(db, db->open_node(db, node->children[node->n]), key);
}

struct DBT * find_next_key(struct DB *db, struct Node *node, struct DBT *key)
{
	if (node->leaf) {
		struct DBT *previous = (struct DBT *)malloc(2*sizeof(*previous));
		previous[0] = deep_copy(node->keys[0]);
		previous[1] = deep_copy(node->values[0]);
		return previous;
	}
	return find_next_key(db, db->open_node(db, node->children[0]), key);
}