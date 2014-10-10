#include "conf.h"

struct DB *create_block(struct DBC *conf)
{
	printf("Start creating block\n");
	struct DB *res = (struct DB *)malloc(1 * sizeof(struct DB));
	res->n = 0;
	res->leaf = 1;
	res->own_tag = conf->first_empty;
	int i, flag = 1;
	conf->exist_or_not[conf->first_empty] = 1;
	for (i = 0; i < conf->count_blocks && flag; i++) {
		if (!conf->exist_or_not[i]) {
			conf->first_empty = i;
			flag = 0;
		}
	}
	if (flag) {
		printf("Error! Empty space ended\n");
		return NULL;
	}
	printf("Creating block ended\n");
	return res;
}

struct DB * read_block(struct DBC *conf, int block_num)
{
	printf("Start reading block %d\n", block_num);
	struct DB *res = (struct DB *)malloc(1 * sizeof(struct DB));
	res->own_tag = block_num;
	//reading num and references to children
	lseek(conf->fd, block_num * conf->chunk_size, SEEK_SET);
	read(conf->fd, &(res->n), sizeof(int));
	read(conf->fd, &(res->leaf), sizeof(char));
	if (!res->n && !res->leaf) {
		printf("Empty block number %d\n", block_num);
		free(res);
		return NULL;
	}
	res->neighbours = (int *)malloc(sizeof(int) * (res->n + 1));
	int i;
	for (i = 0; i < res->n + 1; i++) {
		read(conf->fd, &res->neighbours[i], sizeof(int));
	}
	
	//reading keys and value
	res->keys = (struct DBT *)malloc(sizeof(struct DBT) * (res->n));
	for (i = 0; i < res->n; i++) {
		read(conf->fd, &((res->keys[i]).size), sizeof(char));
		read(conf->fd, &((res->keys[i]).data), res->keys->size);
	}
	res->values = (struct DBT *)malloc(sizeof(struct DBT) * (res->n));
	for (i = 0; i < res->n; i++) {
		read(conf->fd, &(res->values[i]).size, sizeof(char));
		read(conf->fd, &(res->values[i]).data, res->keys->size);
	}
	printf("End reading block %d\n", block_num);
	return res;
}

void write_block(struct DB *x, const struct DBC *conf)
{
	printf("Disk_write starting\n");
	lseek(conf->fd, conf->chunk_size * x->own_tag, SEEK_SET);

	int i;
	write(conf->fd, &x->n, sizeof(int));
	write(conf->fd, &x->leaf, sizeof(char));

	for (i = 0; i < x->n + 1; i++) {
		write(conf->fd, &x->neighbours[i], sizeof(int));
	}
	//printf("%d\n", x->n);
	for (i = 0; i < x->n; i++) {
		write(conf->fd, &((x->keys[i]).size), sizeof(char));
		write(conf->fd, ((x->keys[i]).data), x->keys->size);
		//printf("%d\n", *(int *)x->keys[i].data);
	}
	for (i = 0; i < x->n; i++) {
		write(conf->fd, &(x->values[i]).size, sizeof(char));
		write(conf->fd, &(x->values[i]).data, x->keys->size);
	}
	printf("Disk_write ended\n");
	return;
}