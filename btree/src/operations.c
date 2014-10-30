#include "operations.h"
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
	if (x->node->n == 2 * x->t - 1) {
		struct DB *z = dbcreate(x->disk->file, x->disk->conf);
		if (z == NULL) {
			printf("Cant create block\n");
			return -1;
		}
		x->disk->root_offset = z->own_tag;
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
	while (i < x->node->n && memcmp(key->data, x->node->keys[i].data, key->size) > 0) {
		i++;
	}
	if (i < x->node->n && !memcmp(key->data, x->node->keys[i].data, key->size)){
		data = x->node->values[i].data;
		return 0;
	} else if (x->node->leaf) {
		data = 0;
		return 1;
	} else {
		struct Node *res = x->disk->read_block(i);
		struct Disk *disk = read_disk(x->disk->file, x->disk->conf);
		struct DB *res = (struct DB *)malloc(sizeof(*res));
		res->node = node;
		res->disk = disk;
		return b_tree_search(res, key, data);
	}
}
int close_node(struct Node *x)
{
	printf("close for node is starting\n");
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
	printf("close for node is ending\n");
	return 0;
}
int b_tree_close(struct DB *x)
{
	printf("close is starting\n");
	close_node(x->node);
	write_disk(x->disk);
	free(x->disk->exist_or_not);
	free(x);
	printf("close is ending\n");
	return 0;
}
/*
void delete_elem(struct DB *x, int index)
{
	memmove(x->keys[index], x->keys[index+1], x->n-index-1);
	memmove(x->values[index], x->values[index+1], x->n-index-1);
	return;
}
struct DBT * b_tree_search_node(struct DB *x, const struct DBT *key)
{
	int i = 0;
	while (i < x->n && memcmp(key->data, x->keys[i].data, key->size) > 0) {
		i++;
	}
	if (i < x->n && !memcmp(key->data, x->keys[i].data, key->size)){
		return x;
	} else if (x->leaf) {
		return NULL;
	} else {
		struct DB *res = read_block(x->conf, i);
		return b_tree_search(res, key, data);
	}
}*/
/*
int b_tree_delete(struct DB *x, const struct DBT *key)
{
	struct DB *res = b_tree_search_node(x, key);
	if (res == NULL) return 1;
	x->db_close(x);
	int i = 0;
	while (i < res->n && memcmp(key->data, res->keys[i].data, key->size) > 0) {
		i++;
	}
	if (i < res->n && !memcmp(key->data, res->keys[i].data, key->size) && res->leaf){
		delete_elem(x, i); 
		return 0;
	}
	if(i < res->n && !memcmp(key->data, res->keys[i].data, key->size) && !res->leaf) {
		struct DB *t = read_block(x->conf, res->neighbours[i]);
		if (t->n >= conf->t) {
			struct DBT *t_key = t->keys[t->n-1];
		}
		
	}

}*/