#include "btree.h"
#include "operations.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

struct Node * read_block(int block_num)
{
	//printf("Start reading block %d\n", block_num);
	struct Node *res = (struct Node *)malloc(1 * sizeof(struct Node));
	res->own_tag = block_num;
	//reading num and references to children
	int fd = open(file, O_RDWR);
	lseek(fd, block_num * chunk_size, SEEK_SET);
	read(fd, &res->n, sizeof(int));
	read(fd, &res->leaf, sizeof(char));

	if (!res->n && !res->leaf) {
		free(res);
		return NULL;
	}
	int i;
	//reading keys and value
	res->keys = (struct DBT *)malloc(sizeof(struct DBT) * (res->n));
	for (i = 0; i < res->n; i++) {
		read(fd, &res->keys[i].size, sizeof(char));
		res->keys[i].data = (void *)malloc(sizeof(void *) * (res->keys[i].size));
		read(fd, res->keys[i].data, res->keys[i].size);
	}
	res->values = (struct DBT *)malloc(sizeof(struct DBT) * (res->n));
	for (i = 0; i < res->n; i++) {
		read(fd, &res->values[i].size, sizeof(char));
		res->values[i].data = (void *)malloc(sizeof(void *) * (res->values[i].size));
		read(fd, res->values[i].data, res->values[i].size);
	}
	if (!res->leaf) {
		res->neighbours = (int *)malloc(sizeof(int) * (res->n + 1));
		for (i = 0; i < res->n + 1; i++) {
			read(fd, &res->neighbours[i], sizeof(int));
		}
	}
	close(fd);
	//printf("End reading block %d\n", block_num);
	return res;
}

void write_block(struct Node *res)
{
	printf("Disk_write starting in block %d\n", res->own_tag);
	//start writing blocks
	int fd = open(file, O_RDWR);
	lseek(fd, chunk_size * res->own_tag, SEEK_SET);
	int i;
	write(fd, &res->n, sizeof(int));
	write(fd, &res->leaf, sizeof(char));

	for (i = 0; i < res->n; i++) {
		write(fd, &res->keys[i].size, sizeof(char));
		write(fd, res->keys[i].data, res->keys[i].size);
	}
	for (i = 0; i < res->n; i++) {
		write(fd, &res->values[i].size, sizeof(char));
		write(fd, res->values[i].data, res->values[i].size);
	}
	if(!res->leaf) {
		for (i = 0; i < res->n + 1; i++) {
			write(fd, &res->neighbours[i], sizeof(int));
		}
	}
	printf("Disk_write ended\n");
	close(fd);
	return;
}