#include "operations.h"
#include "btree.h"
#include "block.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void b_tree_split_child(struct DB *x, int index)
{
	printf("B_tree_split_child starting\n");
	struct DBC conf; conf.db_size = x->disk->db_size; conf.chunk_size = x->disk->chunk_size;
	struct DB *z = dbcreate(x->disk->file, &conf);
	z->node->parent = x->node->own_tag;
	struct DB *y = dbopen_index(x->disk->file, &conf, index);
	z->node->leaf = y->node->leaf;
	z->node->n = x->t - 1;
	z->node->keys = (struct DBT *)calloc(z->node->n, sizeof(*z->node->keys));
	z->node->values = (struct DBT *)calloc(z->node->n, sizeof(*z->node->values));
	if (!z->node->leaf) z->node->neighbours = (int *)calloc(z->node->n + 1, sizeof(*z->node->neighbours)*(z->node->n+1));
	int j;
	for (j = 0; j < x->t - 1; j++) {
		//wrong i think
		z->node->keys[j] = y->node->keys[j + x->t];
		z->node->values[j] = y->node->values[j + x->t];
	}
	if (!y->node->leaf) {
		for (j = 0; j < x->t; j++) {
			z->node->neighbours[j] = y->node->neighbours[j + x->t];
		}
		y->node->neighbours = (int *)realloc(y->node->neighbours, sizeof(*y->node->neighbours)*(x->t));
	}
	y->node->n = x->t - 1;
	x->node->keys = (struct DBT *)realloc(x->node->keys, sizeof(*x->node->keys)*(x->node->n+1));
	x->node->values = (struct DBT *)realloc(x->node->values, sizeof(*x->node->values)*(x->node->n+1));
	x->node->neighbours = (int *)realloc(x->node->neighbours, sizeof(*x->node->neighbours)*(x->node->n+2));
	for (j = x->node->n; j >= x->node->n/2 + x->node->n%2; j--) {
		x->node->neighbours[j + 1] = x->node->neighbours[j];
	}
	x->node->neighbours[j + 1] = z->node->own_tag;
	for(j = x->node->n - 1; j >= x->node->n/2 + x->node->n%2; j--) {
		x->node->keys[j + 1] = x->node->keys[j];
		x->node->values[j + 1] = x->node->values[j];
	}
	x->node->keys[j] = y->node->keys[x->t-1];
	x->node->values[j] = y->node->values[x->t-1];
	x->node->n++;
	y->node->keys = (struct DBT *)realloc(y->node->keys, sizeof(*z->node->keys) * (y->node->n));
	y->node->values = (struct DBT *)realloc(y->node->values, sizeof(*z->node->values) * (y->node->n));
	db_close(z);
	db_close(y);
	return;
}
void b_tree_insert_nonfull(struct DB *x, const struct DBT *key, const struct DBT *value)
{
	printf("B_tree_insert_nonfull starting\n");
	x->node->keys = (struct DBT *)realloc(x->node->keys, sizeof(struct DBT) * (x->node->n + 1));
	x->node->values = (struct DBT *)realloc(x->node->values, sizeof(struct DBT) * (x->node->n + 1));
	int i = x->node->n - 1;
	if (x->node->leaf) {
		printf("Ima leaf\n");
		while (i >= 0 && memcmp(key->data, x->node->keys[i].data, key->size) < 0)
		{
			printf("Comparing starting\n");
			x->node->keys[i+1] = x->node->keys[i];
			x->node->values[i+1] = x->node->values[i];
			i--;
		}
		x->node->keys[i+1].data = (void *)calloc(key->size, sizeof(void));
		memcpy(x->node->keys[i+1].data, key->data, key->size);
		x->node->keys[i+1].size = key->size;
		x->node->values[i+1].data = (void *)calloc(value->size, sizeof(void));
		memcpy(x->node->values[i+1].data, value->data, value->size);
		x->node->values[i+1].size = value->size;
		x->node->n++;
	} else {
		printf("ima not leaf\n");
		while(i >= 0 && memcmp(key->data, x->node->keys[i].data, key->size) < 0)
		{
			i--;
		}
		i++;
		struct DBC conf; conf.db_size = x->disk->db_size; conf.chunk_size = x->disk->chunk_size;
		struct DB * res = dbopen_index(x->disk->file, &conf, x->node->neighbours[i]);
		if (res->node->n == 2*x->t - 1) {
			b_tree_split_child(x, i);
			if (memcmp(key->data, x->node->keys[i].data, key->size) > 0) i++;
		}
		b_tree_insert_nonfull(res, key, value);
		db_close(res);
	}
	printf("B_tree_insert_nonfull ended\n");
	return;
}
//todo deep copy
int b_tree_insert(struct DB *x, const struct DBT *key, const struct DBT *value)
{
	printf("Insert starting\n");
	if (x->node->n == 2 * x->t - 1) {
		struct DBC conf; conf.db_size = x->disk->db_size; conf.chunk_size = x->disk->chunk_size;
		struct DB *z = dbcreate(x->disk->file, &conf);
		if (z == NULL) {
			printf("Cant create block\n");
			return -1;
		}
		z->node->n = 1;
		z->node->keys = (struct DBT *)calloc(z->node->n, sizeof(*z->node->keys));
		z->node->values = (struct DBT *)calloc(z->node->n, sizeof(*z->node->values));
		z->node->parent = z->node->own_tag;

		x->disk->root_offset = z->node->own_tag;
		x->node->parent = z->node->own_tag;
		z->node->neighbours = (int *)calloc(z->node->n + 1, sizeof(*z->node->neighbours));
		z->node->neighbours[0] = x->node->own_tag;
		z->node->leaf = 0;
		b_tree_split_child(z, z->node->neighbours[0]);
		b_tree_insert_nonfull(z, key, value);
		db_close(z);
	} else b_tree_insert_nonfull(x, key, value);
	//db_close(x);
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
		*data = x->node->values[i];
		return 0;
	} else if (x->node->leaf) {
		data = NULL;
		return 1;
	} else {
		struct DBC conf; conf.db_size = x->disk->db_size; conf.chunk_size = x->disk->chunk_size;
		struct DB *res = dbopen_index(x->disk->file, &conf, i);
		return b_tree_search(res, key, data);
	}
}
int close_node(struct Disk *y, struct Node *x)
{
	printf("close for node is starting\n");
	y->write_block(y, x);
	int i;
	for(i = 0; i < x->n; i++) {
		free(x->values[i].data);
		free(x->keys[i].data);
	}
	if (x->n) free(x->values);
	if (x->n) free(x->keys);
	if (!x->leaf) free(x->neighbours);
	printf("close for node is ending\n");
	return 0;
}
int b_tree_close(struct DB *x)
{
	printf("close is starting\n");
	close_node(x->disk, x->node);
	write_disk(x->disk);
	free(x->disk);
	free(x->node);
	free(x);
	printf("close is ending\n");
	return 0;
}

void delete_elem(struct DB *x, int index)
{
	int i = 0, j = 0;
	x->node->n--;
	struct DBT *new_keys = (struct DBT *)calloc(x->node->n, sizeof(*new_keys));
	struct DBT *new_values = (struct DBT *)calloc(x->node->n, sizeof(*new_values));
	for (i = 0; i < x->node->n + 1; i++) {
		if (i == index) continue;
		new_keys[j] = x->node->keys[i];
		new_values[j] = x->node->values[i];
		j++;
	}
	free(x->node->keys[index].data);
	free(x->node->values[index].data);
	free(x->node->keys);
	free(x->node->values);
	x->node->keys = new_keys;
	x->node->values = new_values;
	return;
}
//wtf such lazy
void delete_elem_without_free(struct DB *x, int index)
{
	int i = 0, j = 0;
	x->node->n--;
	struct DBT *new_keys = (struct DBT *)calloc(x->node->n, sizeof(*new_keys));
	struct DBT *new_values = (struct DBT *)calloc(x->node->n, sizeof(*new_values));
	for (i = 0; i < x->node->n + 1; i++) {
		if (i == index) continue;
		new_keys[j] = x->node->keys[i];
		new_values[j] = x->node->values[i];
		j++;
	}
	free(x->node->keys);
	free(x->node->values);
	x->node->keys = new_keys;
	x->node->values = new_values;
	return;
}
void delete_neighbour(struct DB *x, int index)
{
	//todo this
	int i = 0, j = 0;
	x->node->n--;
	int *new_neighbours = (int *)calloc(x->node->n, sizeof(*new_neighbours));
	for (i = 0; i < x->node->n + 1; i++) {
		if (i == index) continue;
		new_neighbours[j] = x->node->neighbours[i];
		j++;
	}
	free(x->node->neighbours);
	x->node->neighbours = new_neighbours;
	return;
}
void unite(struct DB *x, struct DB *y) 
{
	if (y == NULL) return;
	x->node->keys = (struct DBT *)realloc(x->node->keys, sizeof(*x->node->keys)*(x->node->n+y->node->n));
	x->node->values = (struct DBT *)realloc(x->node->values, sizeof(*x->node->values)*(x->node->n+y->node->n));
	int i = 0;
	while (i < y->node->n) {
		x->node->keys[i+x->node->n] = y->node->keys[i];
		x->node->values[i+x->node->n] = y->node->values[i];
		i++;
	}
	//maybe error
	x->node->neighbours = (int *)realloc(x->node->neighbours, sizeof(*x->node->neighbours)*(x->node->n+y->node->n+2));
	i = 0;
	while (i < y->node->n+1) {
		x->node->neighbours[i+x->node->n+1] = y->node->neighbours[i];
		i++;
	}
	x->node->n += y->node->n;
	return;
}
int b_tree_delete(struct DB *x, const struct DBT *key)
{
	printf("Deleting element start\n");
	struct DBC conf; conf.db_size = x->disk->db_size; conf.chunk_size = x->disk->chunk_size;
	int i = 0;
	while (i < x->node->n && memcmp(key->data, x->node->keys[i].data, key->size) > 0) {
		i++;
	}
	if (i < x->node->n && !memcmp(key->data, x->node->keys[i].data, key->size) && x->node->leaf){
		printf("ima leaf\n");
		if (x->node->n >= x->t) delete_elem(x, i);
		else {
			if (i < x->node->n-1) {
				struct DB *parent = dbopen_index(x->disk->file, &conf, x->node->parent);
				int j = 0;
				while(j < parent->node->n+1 && parent->node->neighbours[j] != x->node->own_tag) {
					j++;
				}
				struct DB *y = NULL;
				if (j < parent->node->n) {
					y = dbopen_index(x->disk->file, &conf, parent->node->neighbours[j+1]);
					if (y->node->n >= x->t) {
						struct DBT *k1 = &y->node->keys[0];
						struct DBT *k1_value = &y->node->values[0];//k1
						struct DBT *k2 = &parent->node->keys[i];//k2
						struct DBT *k2_value = &parent->node->values[i];
						delete_elem(x, i);
						delete_elem(y, 0);
						b_tree_insert(x, k2, k2_value);
						b_tree_insert(x, k1, k1_value);
						db_close(y);
						db_close(parent);
						return 0;
					}
					j++;
				}

				if (j > 1) {
					y = dbopen_index(x->disk->file, &conf, parent->node->neighbours[j-1]);
					if (y->node->n >= x->t) {
						struct DBT *k1 = &y->node->keys[y->node->n-1];
						struct DBT *k1_value = &y->node->values[y->node->n-1];//k1
						struct DBT *k2 = &parent->node->keys[i];//k2
						struct DBT *k2_value = &parent->node->values[i];
						delete_elem(x, i);
						delete_elem(y, y->node->n-1);
						delete_elem(parent, i);
						b_tree_insert(x, k2, k2_value);
						b_tree_insert(x, k1, k1_value);
						db_close(y);
						db_close(parent);
						return 0;
					}
					j--;
				}
				struct DBT *k2 = &parent->node->keys[i];//k2
				struct DBT *k2_value = &parent->node->values[i];
				unite(x, y);
				db_close(y);
				delete_neighbour(parent, j);
				delete_elem(x, i);
				delete_elem(parent, i);
				b_tree_insert(x, k2, k2_value);
				parent->disk->exist_or_not[j] = 0;
				db_close(parent);
			}
		}
		printf("Deleting element end\n");
		return 0;
	}
	if(i < x->node->n && !memcmp(key->data, x->node->keys[i].data, key->size) && !x->node->leaf) {
		printf("ima not a leaf\n");
		struct DB *y = dbopen_index(x->disk->file, &conf, x->node->neighbours[i+1]);
		if (y->node->n >= x->t) {
			struct DBT *y_key = &y->node->keys[y->node->n-1];
			struct DBT *y_value = &y->node->values[y->node->n-1];//k1
			//maybe btreedelete
			delete_elem_without_free(y, y->node->n-1);
			x->node->keys[i] = *y_key;
			x->node->values[i] = *y_value;
			return 0;
		} 
		struct DB *y1 = dbopen_index(x->disk->file, &conf, x->node->neighbours[i]);
		if (y1->node->n >= x->t) {
			struct DBT *y_key = &y->node->keys[0];
			struct DBT *y_value = &y->node->values[0];
			//maybe btreedelete
			delete_elem_without_free(y1, 0);
			x->node->keys[i] = *y_key;
			x->node->values[i] = *y_value;
			return 0;
		} else {
			unite(y, y1);
			db_close(y1);
			b_tree_insert(y, key, &x->node->values[i]);
			delete_elem(x, i);
			y->disk->exist_or_not[i] = 0;
			delete_neighbour(x, i);
			db_close(y);
		}	
	} else {
		struct DB *y = dbopen_index(x->disk->file, &conf, x->node->neighbours[i]);
		b_tree_delete(y, key);
		db_close(y);
	}
	printf("Deleting element end\n");
	return 0;
}