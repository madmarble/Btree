#include "btree.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "operations.h"
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
void initialize_fields(struct Disk *res)
{
	res->start_offset = 2 * sizeof(size_t) + sizeof(int);
	res->start_offset += res->db_size / res->chunk_size + res->db_size % res->chunk_size;
	res->start_offset = res->start_offset / res->chunk_size + (res->start_offset % res->chunk_size ? 1 : 0);
	res->count_blocks = res->db_size / res->chunk_size;
	return;
}
void write_disk(struct Disk *disk)
{
	int fd = open(disk->file, O_RDWR);
	//write(fd, &disk->db_size, sizeof(disk->db_size));
	//write(fd, &disk->chunk_size, sizeof(disk->db_size));
	int i;
	for (i = 0; i < disk->db_size / disk->chunk_size; i++) {
		write(fd, &disk->exist_or_not[i], sizeof(char));
	}
	write(fd, &disk->root_offset, sizeof(disk->root_offset));
	close(fd);
	return;
}
struct Disk * create_disk(const char *file)
{
	struct Disk *res = (struct Disk *)malloc(sizeof(*res));
	struct DBC *conf = (struct DBC *)malloc(sizeof(*conf));
	conf->db_size = 512 * 1024 * 1024;
	conf->chunk_size = 4 * 1024;
	res->conf = conf;
	initialize_fields(res);
	res->disk->first_empty = res->disk->start_offset;
	res->disk->root_offset = res->disk->start_offset;
	res->disk->exist_or_not = (char *)calloc(res->disk->count_blocks, sizeof(char));
	res->file = file;
	return res;
}
struct Disk * read_disk(const char *file, struct DBC *conf)
{
	if (!conf) {
		return create_disk(file);
	}

	int fd = open(file, O_RDWR);
	struct Disk *res = (struct Disk *)malloc(sizeof(*res));
	//read(fd, &disk->db_size, sizeof(disk->db_size));
	//read(fd, &disk->chunk_size, sizeof(disk->db_size));
	res->conf = conf;
	int i;
	for (i = 0; i < res->db_size / res->chunk_size; i++) {
		read(fd, &res->exist_or_not[i], sizeof(char));
	}
	read(fd, &res->root_offset, sizeof(res->root_offset));

	initialize_fields(res);
	res->file = file;
	close(fd);
	return res;
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
	printf("Creating DB ended\n");
	return res;
}

struct DB * dbopen(const char *file, struct DBC *conf)
{
	int fd = open(file, O_RDWR);
	if (fd == -1) {
		return dbcreate(file, conf);
	} else {
		struct Disk *disk = read_disk(file, conf);
		struct Node *node = disk->read_block(root_offset);
		struct DB * res = (struct DB *)malloc(sizeof(struct DB *) * 1);
		res->node = node;
		res->disk = disk;
		return res;
	}
}