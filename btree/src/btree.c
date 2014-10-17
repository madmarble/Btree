#include "btree.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "block.h"
#include "operations.h"
int db_close(struct DB *db) {
	return db->close(db);
}/*
int db_del(struct DB *db, void *key, size_t key_len) {
	struct DBT keyt = {
		.data = key,
		.size = key_len
	};
	return db->del(db, &keyt);
}*/
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
void write_conf(struct DBC *conf)
{
	lseek(conf->fd, 0, SEEK_SET);
	write(conf->fd, &conf->db_size, sizeof(conf->db_size));
	write(conf->fd, &conf->chunk_size, sizeof(conf->chunk_size));
	int i;
	for (i = 0; i < conf->db_size / conf->chunk_size; i++) {
		write(conf->fd, &conf->exist_or_not[i], sizeof(char));
	}
	write(conf->fd, &conf->root_offset, sizeof(conf->root_offset));
	return;
}
void read_conf(struct DBC *conf)
{
	lseek(conf->fd, 0, SEEK_SET);
	read(conf->fd, &conf->db_size, sizeof(conf->db_size));
	read(conf->fd, &conf->chunk_size, sizeof(conf->chunk_size));
	int i;
	for (i = 0; i < conf->db_size / conf->chunk_size; i++) {
		read(conf->fd, &conf->exist_or_not[i], sizeof(char));
	}
	read(conf->fd, &conf->root_offset, sizeof(conf->root_offset));
	conf->t = conf->chunk_size / 2 - 1;
	conf->start_offset = 2 * sizeof(size_t) + sizeof(int);
	conf->start_offset += conf->db_size / conf->chunk_size + conf->db_size % conf->chunk_size;
	conf->start_offset = conf->start_offset / conf->chunk_size + (conf->start_offset % conf->chunk_size ? 1 : 0);
	conf->count_blocks = conf->db_size / conf->chunk_size;
	return;
}
struct DB * dbcreate(struct DBC *conf)
{
	printf("Start creating block\n");
	struct DB *res = (struct DB *)malloc(1 * sizeof(struct DB));
	res->n = 0;
	res->leaf = 1;
	res->conf = conf;
	int i, flag = 1;
	conf->exist_or_not[conf->first_empty] = 1;
	res->own_tag = conf->first_empty;
	for (i = 0; i < conf->count_blocks && flag; i++) {
		if (!conf->exist_or_not[i]) {
			flag = 0;
			conf->first_empty = i;
		}
	}
	if (flag) {
		printf("Error! Empty space ended\n");
		return NULL;
	}
	res->close = &b_tree_close;
	res->put = &b_tree_insert;
	res->get = &b_tree_search;
	printf("Creating block ended\n");
	return res;
}

struct DB * dbopen(const char *file, struct DBC *conf)
{
	conf->fd = open(file, O_RDWR);
	if (conf->fd == -1) {

		fprintf(stderr, "Cant open file %s!\n", file);
		conf->fd = open(file, O_RDWR | O_CREAT);

		if (conf->fd == -1) {
			fprintf(stderr, "Cant create file %s!\n", file);
			return NULL;
		}

		//initialize settings
		conf->db_size = 512 * 1024 * 1024;
		conf->chunk_size = 4 * 1024;
		conf->t = conf->chunk_size / 2 - 1;
		conf->start_offset = 2 * sizeof(size_t) + sizeof(int);
		conf->start_offset += conf->db_size / conf->chunk_size + conf->db_size % conf->chunk_size;
		conf->start_offset = conf->start_offset / conf->chunk_size + (conf->start_offset % conf->chunk_size ? 1 : 0);
		conf->count_blocks = conf->db_size / conf->chunk_size;
		conf->first_empty = conf->start_offset;
		conf->root_offset = conf->start_offset;
		conf->exist_or_not = (char *)calloc(conf->count_blocks, sizeof(char));

		struct DB *res = dbcreate(conf);
		//write_conf(struct DBC *conf);
		return res;
	} else {
		read_conf(conf);
		return read_block(conf, conf->root_offset);
	}
}