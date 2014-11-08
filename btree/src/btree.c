#include "btree.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "operations.h"
#include "block.h"
int db_close(struct DB *db) {
	return db->close(db);
}
int db_del(struct DB *db, void *key, size_t key_len) {
	struct DBT keyt = {
		.data = key,
		.size = key_len
	};
	return db->del(db, &keyt);
}
int db_get(struct DB *db, void *key, size_t key_len, void **val, size_t *val_len) {
	struct DBT keyt = {
		.data = key,
		.size = key_len
	};
	struct DBT valt = {0, 0};
	int rc = db->get(db, &keyt, &valt);
	*val = valt.data;
	*val_len = valt.size;
	return rc;
}
int db_put(struct DB *db, void *key, size_t key_len, void *val, size_t val_len) {
	struct DBT keyt = {
		.data = key,
		.size = key_len
	};
	struct DBT valt = {
		.data = val,
		.size = val_len
	};
	return db->put(db, &keyt, &valt);
}
struct DB * dbcreate(const char *file, struct DBC *conf)
{
	printf("Start creating DB\n");
	struct DB *res = (struct DB *)malloc(1 * sizeof(struct DB));
	res->node = (struct Node *)malloc(1 * sizeof(struct Node));
	res->node->n = 0;
	res->node->leaf = 1;

	res->disk = read_disk(file, conf);
	int i, flag = 1;
	res->disk->exist_or_not[res->disk->first_empty] = 1;
	res->node->own_tag = res->disk->first_empty;
	for (i = 0; i < res->disk->count_blocks && flag; i++) {
		if (!res->disk->exist_or_not[i]) {
			flag = 0;
			res->disk->first_empty = i;
		}
	}
	if (flag) {
		printf("Error! Empty space ended\n");
		return NULL;
	}
	res->close = &b_tree_close;
	res->put = &b_tree_insert;
	res->get = &b_tree_search;
	res->del = &b_tree_delete;
	res->t = res->disk->db_size/res->disk->chunk_size + 1; //maybe error
	printf("Creating DB ended\n");
	return res;
}

struct DB * dbopen(const char *file, struct DBC *conf)
{
	printf("Start open btree\n");
	int fd = open(file, O_RDWR);
	if (fd == -1) {
		return dbcreate(file, conf);
	} else {
		struct Disk *disk = read_disk(file, conf);
		struct Node *node = disk->read_block(disk, disk->root_offset);
		struct DB * res = (struct DB *)calloc(1, sizeof(*res));
		res->node = node;
		res->disk = disk;
		res->close = &b_tree_close;
		res->put = &b_tree_insert;
		res->get = &b_tree_search;
		res->del = &b_tree_delete;
		res->t = res->disk->db_size/res->disk->chunk_size + 1; //maybe error
		res->node->parent = res->node->own_tag;
		printf("End open btree\n");
		return res;
	}
}
struct DB * dbopen_index(const char *file, struct DBC *conf, int index)
{
	printf("Start open btree\n");
	int fd = open(file, O_RDWR);
	if (fd == -1) {
		return dbcreate(file, conf);
	} else {
		struct Disk *disk = read_disk(file, conf);
		struct Node *node = disk->read_block(disk, index);
		struct DB * res = (struct DB *)calloc(1, sizeof(*res));
		res->node = node;
		res->disk = disk;
		res->close = &b_tree_close;
		res->put = &b_tree_insert;
		res->get = &b_tree_search;
		res->del = &b_tree_delete;
		res->t = res->disk->db_size/res->disk->chunk_size + 1; //maybe error
		printf("End open btree\n");
		return res;
	}
}
