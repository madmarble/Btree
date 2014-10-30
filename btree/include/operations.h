#pragma once
#include "btree.h"
void b_tree_split_child(struct DB *x, struct DB *y);
void b_tree_insert_nonfull(struct DB *x, const struct DBT *key, const struct DBT *value);
int b_tree_insert(struct DB *root, const struct DBT *key, const struct DBT *value);
int b_tree_search(struct DB *x, const struct DBT *key, struct DBT *data);
int b_tree_close(struct DB *x);

/*
int main(int argc, char **argv)
{
	printf("Starting programm\n");

	printf("Start creating settings\n");
	conf = (struct DBC *)calloc(1, sizeof(struct DBC));
	if (dbopen(argv[1], conf)) {
		dbcreate(argv[1], conf);
	}

	lseek(conf->fd, conf->start_offset * conf->chunk_size, SEEK_SET);
	printf("Start reading blocks from file %s\n", argv[1]);
	int i;
	for(i = conf->start_offset; i < conf->count_blocks; i++) {
		struct DB * res = read_block(conf, i);
		if (res == NULL) {
			conf->exist_or_not[i] = 0;
			//printf("Block number %d empty\n", i);
			if (conf->first_empty == -1) conf->first_empty = i;
		} else {
			conf->exist_or_not[i] = 1;
			printf("Block number %d busy\n", i);
			db_close(res);
		}
	}
	if (conf->first_empty == -1) {
		printf("Free space ended!\n");
		return -1;
	}
	short int k = 11;
	int p = 9;
	struct DB *root = read_block(conf, conf->root_offset);
	if (root == NULL) root = create_block(conf);
	making_function(root);
	db_put(root, &k, sizeof(k), &p, sizeof(p));
	close(conf->fd);
	free(conf);
	return 0;
}*/