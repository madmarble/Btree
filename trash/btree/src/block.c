#include "btree.h"
#include "operations.h"
#include "block.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

struct Node * read_block(struct Disk *x, int block_num)
{
	printf("Start reading block %d\n", block_num);
	struct Node *res = (struct Node *)calloc(1, sizeof(struct Node));
	res->own_tag = block_num;
	//reading num and references to children
	int fd = open(x->file, O_RDWR);
	lseek(fd, block_num * x->chunk_size, SEEK_SET);
	read(fd, &res->n, sizeof(int));
	//printf("N is %d\n", res->n);
	read(fd, &res->leaf, sizeof(char));
	read(fd, &res->parent, sizeof(char));
	if (!res->n && !res->leaf) {
		free(res);
		return NULL;
	}
	int i;
	//reading keys and value
	res->keys = (struct DBT *)calloc(res->n, sizeof(*res->keys));
	for (i = 0; i < res->n; i++) {
		read(fd, &res->keys[i].size, sizeof(char));
		printf("Size for key is %d\n", (char)res->keys[i].size);
		res->keys[i].data = (void *)calloc(res->keys[i].size, sizeof(void));
		read(fd, res->keys[i].data, res->keys[i].size);
		printf("Data for key is %d\n", *(int *)res->keys[i].data);
	}
	res->values = (struct DBT *)calloc(res->n, sizeof(*res->values));
	for (i = 0; i < res->n; i++) {
		read(fd, &res->values[i].size, sizeof(char));
		printf("Size for value is %d\n", (char)res->values[i].size);
		res->values[i].data = (void *)calloc(res->values[i].size, sizeof(void));
		read(fd, res->values[i].data, res->values[i].size);
		printf("Data for value is %d\n", *(int *)res->values[i].data);
	}
	if (!res->leaf) {
		res->neighbours = (int *)calloc(res->n + 1, sizeof(*res->neighbours));
		for (i = 0; i < res->n + 1; i++) {
			read(fd, &res->neighbours[i], sizeof(int));
		}
	}
	close(fd);
	//printf("This is %d\n", *(int *)res->keys[0].data);
	printf("End reading block %d\n", block_num);
	return res;
}

void write_block(struct Disk *x, struct Node *res)
{
	printf("Disk_write starting in block %d\n", res->own_tag);
	//start writing blocks
	int fd = open(x->file, O_RDWR);
	lseek(fd, x->chunk_size * res->own_tag, SEEK_SET);
	int i;
	write(fd, &res->n, sizeof(int));
	write(fd, &res->leaf, sizeof(char));
	write(fd, &res->parent, sizeof(char));
	for (i = 0; i < res->n; i++) {
		//printf("Size is %d\n", (int)res->keys[i].size);
		write(fd, &res->keys[i].size, sizeof(char));
		write(fd, res->keys[i].data, res->keys[i].size);
	}
	//printf("This is %d\n", *(int *)res->keys[0].data);
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

void initialize_fields(struct Disk *res)
{
	//maybe error
	res->start_offset = 2 * sizeof(size_t);
	res->start_offset += res->db_size / res->chunk_size + res->db_size % res->chunk_size;
	res->start_offset = res->start_offset / res->chunk_size + (res->start_offset % res->chunk_size ? 1 : 0);
	res->count_blocks = res->db_size / res->chunk_size;
	return;
}
void write_disk(struct Disk *disk)
{
	printf("Start writing disk\n");
	int fd = open(disk->file, O_RDWR);
	write(fd, &disk->db_size, sizeof(disk->db_size));
	//printf("db_size is %d\n", disk->db_size);
	write(fd, &disk->chunk_size, sizeof(disk->db_size));
	//printf("chunk_size is %d\n", disk->chunk_size);
	int i;
	for (i = 0; i < disk->count_blocks; i++) {
		write(fd, &disk->exist_or_not[i], sizeof(char));
	}
	//printf("Size of exist_or_not is %d\n", disk->count_blocks);
	write(fd, &disk->root_offset, sizeof(disk->root_offset));
	//printf("Root offset is %d\n", disk->root_offset);
	free(disk->exist_or_not);
	close(fd);
	printf("End writing disk\n");
	return;
}
struct Disk * create_disk(const char *file)
{
	printf("Start creating disk\n");
	struct Disk *res = (struct Disk *)calloc(1, sizeof(*res));
	res->db_size = 512 * 1024 * 1024;
	res->chunk_size = 4 * 1024;
	initialize_fields(res);
	res->first_empty = res->start_offset;
	res->root_offset = res->start_offset;
	//printf("Root offset is %d\n", res->root_offset);
	res->exist_or_not = (char *)calloc(res->count_blocks, sizeof(char));
	res->file = file;
	res->read_block = &read_block;
	res->write_block = &write_block;
	int fd = open(file, O_CREAT | O_RDWR);
	close(fd);
	printf("End creating disk\n");
	return res;
}
struct Disk * read_disk(const char *file, struct DBC *conf)
{
	printf("Start reading disk\n");
	int fd = open(file, O_RDWR);
	if (fd == -1) {
		return create_disk(file);
	}
	struct Disk *res = (struct Disk *)calloc(1, sizeof(*res));
	read(fd, &res->db_size, sizeof(res->db_size));
	//printf("db_size is %d\n", res->db_size);
	read(fd, &res->chunk_size, sizeof(res->chunk_size));
	//printf("chunk_size is %d\n", res->chunk_size);
	initialize_fields(res);
	res->exist_or_not = (char *)calloc(res->count_blocks, sizeof(*res->exist_or_not));
	int i;
	for (i = 0; i < res->count_blocks; i++) {
		read(fd, &res->exist_or_not[i], sizeof(char));
	}
	//printf("Size of exist_or_not is %d\n", res->count_blocks);
	read(fd, &res->root_offset, sizeof(res->root_offset));
	//printf("Root offset is %d\n", res->root_offset);
	res->file = file;
	res->read_block = &read_block;
	res->write_block = &write_block;
	close(fd);
	printf("End reading disk\n");
	return res;
}