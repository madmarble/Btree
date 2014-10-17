#include "operations.h"
#include "block.h"
#include "btree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void b_tree_split_child(struct DB *x, struct DB *y)
{
	printf("B_tree_split_child starting\n");
	struct DB *z = dbcreate(x->conf);
	z->leaf = y->leaf;
	z->n = x->conf->t - 1;
	z->keys = (struct DBT *)malloc(sizeof(struct DBT) * (z->n));
	z->values = (struct DBT *)malloc(sizeof(struct DBT) * (z->n));
	z->neighbours = (int *)malloc(sizeof(int) * (z->n + 1));
	int j;
	for (j = 0; j < x->conf->t - 1; j++) {
		z->keys[j] = y->keys[j + x->conf->t];
		z->values[j] = y->values[j + x->conf->t];
	}
	if (!y->leaf) {
		for (j = 0; j < x->conf->t; j++) {
			z->neighbours[j] = y->neighbours[j + x->conf->t];
		}
	}
	y->n = x->conf->t - 1;
	for (j = x->n; j >= x->n/2 + x->n%2; j--) {
		x->neighbours[j + 1] = x->neighbours[j];
	}
	x->neighbours[j + 1] = z->own_tag;
	for(j = x->n - 1; j >= x->n/2 + x->n%2; j--) {
		x->keys[j + 1] = x->keys[j];
	}
	x->keys[j] = y->keys[x->conf->t - 1];
	x->n++;
	db_close(z);
	return;
}
void b_tree_insert_nonfull(struct DB *x, const struct DBT *key, const struct DBT *value)
{
	printf("B_tree_insert_nonfull starting\n");
	x->keys = (struct DBT *)realloc(x->keys, sizeof(struct DBT) * (x->n + 1));
	x->values = (struct DBT *)realloc(x->values, sizeof(struct DBT) * (x->n + 1));
	int i = x->n - 1;
	if (x->leaf) {
		while (i >= 0 && memcmp(key->data, x->keys[i].data, key->size) < 0)
		{
			printf("Comparing starting\n");
			x->keys[i+1] = x->keys[i];
			x->values[i+1] = x->values[i];
			i--;
		}
		x->keys[i+1] = *key;
		x->values[i+1] = *value;
		x->n++;
	} else {
		while(i >= 0 && memcmp(key->data, x->keys[i].data, key->size) < 0)
		{
			i--;
		}
		i++;
		struct DB * res = read_block(x->conf, x->neighbours[i]);
		if (res->n == 2*x->conf->t - 1) {
			b_tree_split_child(x, res);
			if (memcmp(key->data, x->keys[i].data, key->size) > 0) i++;
		}
		b_tree_insert_nonfull(res, key, value);
		db_close(res);
	}
	printf("B_tree_insert_nonfull ended\n");
	return;
}
int b_tree_insert(struct DB *x, const struct DBT *key, const struct DBT *value)
{
	printf("Insert starting\n");
	if (x->n == 2 * x->conf->t - 1) {
		struct DB *z = dbcreate(x->conf);
		if (z == NULL) {
			printf("Cant create block\n");
			return -1;
		}
		x->conf->root_offset = z->own_tag;
		z->neighbours = (int *)malloc(sizeof(int) * (z->n + 1));
		z->neighbours[0] = x->own_tag;
		z->leaf = 0;
		struct DB *y = read_block(x->conf, x->conf->root_offset);
		b_tree_split_child(z, y);
		b_tree_insert_nonfull(z, key, value);
		db_close(z);
		db_close(y);
	} else b_tree_insert_nonfull(x, key, value);
	db_close(x);
	printf("Insert ended\n");
	return 0;
}
int b_tree_search(struct DB *x, const struct DBT *key, struct DBT *data)
{
	int i = 0;
	while (i < x->n && memcmp(key->data, x->keys[i].data, key->size) > 0) {
		i++;
	}
	if (i < x->n && !memcmp(key->data, x->keys[i].data, key->size)){
		data = x->values[i].data;
		return 0;
	} else if (x->leaf) {
		data = 0;
		return 1;
	} else {
		struct DB *res = read_block(x->conf, i);
		return b_tree_search(res, key, data);
	}
}
int b_tree_close(struct DB *x)
{
	printf("close is starting\n");
	write_block(x);
	//int i;
	//still dont understand why :(
	/*for(i = 0; i < x->n; i++) {
		free(x->values[i].data);
		free(x->keys[i].data);
	}*/
	if (x->n) free(x->values);
	if (x->n) free(x->keys);
	if (!x->leaf) free(x->neighbours);
	free(x);
	printf("close is ending\n");
	return 0;
}