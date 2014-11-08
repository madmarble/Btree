#pragma once
#include "btree.h"
void b_tree_split_child(struct DB *x, int index);
void b_tree_insert_nonfull(struct DB *x, const struct DBT *key, const struct DBT *value);
int b_tree_insert(struct DB *root, const struct DBT *key, const struct DBT *value);
int b_tree_search(struct DB *x, const struct DBT *key, struct DBT *data);
int b_tree_close(struct DB *x);
int b_tree_delete(struct DB *x, const struct DBT *key);