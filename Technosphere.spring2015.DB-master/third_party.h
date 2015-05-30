#pragma once
#include "block_api.h"

int new_memcmp(struct DBT t1, struct DBT t2);
int make_realloc(struct Node *node);
struct DBT deep_copy(struct DBT resource);

int find_key(struct Node *node, struct DBT *key);
int delete_key(struct Node *node, int index);

struct DBT * find_previous_key(struct DB *db, struct Node *node, 
	struct DBT *key);

struct DBT * find_next_key(struct DB *db, struct Node *node, 
	struct DBT *key);