#include "block.h"
#include "btree.h"
#include "operations.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

struct DB * read_block(struct DBC *conf, int block_num)
{
	//printf("Start reading block %d\n", block_num);
	struct DB *res = (struct DB *)malloc(1 * sizeof(struct DB));
	res->own_tag = block_num;
	res->conf = conf;
	//reading num and references to children
	lseek(res->conf->fd, block_num * res->conf->chunk_size, SEEK_SET);
	read(res->conf->fd, &res->n, sizeof(int));
	read(res->conf->fd, &res->leaf, sizeof(char));

	if (!res->n && !res->leaf) {
		free(res);
		return NULL;
	}
	int i;
	if (!res->leaf) {
		res->neighbours = (int *)malloc(sizeof(int) * (res->n + 1));
		for (i = 0; i < res->n + 1; i++) {
			read(res->conf->fd, &res->neighbours[i], sizeof(int));
		}
	}
	
	//reading keys and value
	res->keys = (struct DBT *)malloc(sizeof(struct DBT) * (res->n));
	for (i = 0; i < res->n; i++) {
		read(res->conf->fd, &res->keys[i].size, sizeof(char));
		res->keys[i].data = (void *)malloc(sizeof(void *) * (res->keys[i].size));
		read(res->conf->fd, res->keys[i].data, res->keys[i].size);
	}
	res->values = (struct DBT *)malloc(sizeof(struct DBT) * (res->n));
	for (i = 0; i < res->n; i++) {
		read(res->conf->fd, &res->values[i].size, sizeof(char));
		res->values[i].data = (void *)malloc(sizeof(void *) * (res->values[i].size));
		read(res->conf->fd, res->values[i].data, res->values[i].size);
	}
	res->close = &b_tree_close;
	res->put = &b_tree_insert;
	res->get = &b_tree_search;
	//printf("End reading block %d\n", block_num);
	return res;
}

void write_block(struct DB *res)
{
	printf("Disk_write starting in block %d\n", res->own_tag);
	//start writing blocks
	lseek(res->conf->fd, res->conf->chunk_size * res->own_tag, SEEK_SET);
	int i;
	write(res->conf->fd, &res->n, sizeof(int));
	write(res->conf->fd, &res->leaf, sizeof(char));

	if(!res->leaf) {
		for (i = 0; i < res->n + 1; i++) {
			write(res->conf->fd, &res->neighbours[i], sizeof(int));
		}
	}
	for (i = 0; i < res->n; i++) {
		write(res->conf->fd, &res->keys[i].size, sizeof(char));
		write(res->conf->fd, res->keys[i].data, res->keys[i].size);
	}
	for (i = 0; i < res->n; i++) {
		write(res->conf->fd, &res->values[i].size, sizeof(char));
		write(res->conf->fd, res->values[i].data, res->values[i].size);
	}
	write_conf(res->conf);
	printf("Disk_write ended\n");
	return;
}