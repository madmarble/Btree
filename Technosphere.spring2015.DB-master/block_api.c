#include "block_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int MAX_CACHE_SIZE = 4;
int init(struct bitmap *bitmap, int n)
{
	char rest;
	if (n % 8) rest = 1;
	else rest = 0;
	bitmap->N = n/8 + rest;
	bitmap->bitmask = (unsigned char *)calloc(bitmap->N, sizeof(*bitmap->bitmask));
	return 0;
}
int ffree(struct bitmap *bitmap)
{
	//printf("Start free for bitmap\n");
	fflush(stdout);
	free(bitmap->bitmask);
	//printf("End free for bitmap\n");
	fflush(stdout);
	return 0;
}
int set(struct bitmap *bitmap, int i)
{
	int num_block = i/8;
	i %= 8;
	int tmp = 1;
	tmp <<= i;
	bitmap->bitmask[num_block] |= tmp;
	return 0;
}
int unset(struct bitmap *bitmap, int i)
{
	int num_block = i/8;
	i %= 8;
	char tmp = 1;
	tmp <<= i;
	tmp = ~tmp;
	bitmap->bitmask[num_block] &= tmp;
	return 0;
}
int first_empty(struct bitmap *bitmap)
{
	int i = 0;
	int num_block = 0;
	unsigned char tmp = bitmap->bitmask[num_block];
	int free_block = 0;
	while(1) {
		if (tmp%2) {
			tmp /= 2;
			i++;
			free_block++;
		} else{
			return free_block;
		}
		if (i == 8) {
			num_block++;
			tmp = bitmap->bitmask[num_block];
			i = 0;
		}
		if (num_block > bitmap->N) {
			fprintf(stderr, "All file busy. No more free block\n");
			return -1;
		}
	}
	fprintf(stderr, "Something strange happen here\n");
	return -1;
}
int show(struct bitmap *bitmap)
{
	int i;
	printf("N is %d\n", bitmap->N);
	for (i = 0; i < bitmap->N; i++) {
		unsigned char block = (unsigned char)bitmap->bitmask[i];
		int k;
		for (k = 0; k < 8; k++) {
			if ((block>>7) == 1) printf("%d", (unsigned int)(block>>7));
			else printf("0");
			block <<= 1;
		}
		printf(" ");

	}
	printf("\n");

	return 0;
}


int read_block(struct DB *db, struct Node *node)
{
	int i;
	lseek(db->block_api->fd, node->num_vertix*db->block_api->page_size, SEEK_SET);

	//read meta data
	read(db->block_api->fd, &node->n, sizeof(node->n));
	read(db->block_api->fd, &node->leaf, sizeof(node->leaf));

	//read keys, values and children
	node->keys = (struct DBT *)malloc(node->n*sizeof(*node->keys));
	node->values = (struct DBT *)malloc(node->n*sizeof(*node->values));
	for(i = 0; i < node->n; i++) {
		read(db->block_api->fd, &node->keys[i].size, sizeof(node->keys[i].size));
		node->keys[i].data = calloc(node->keys[i].size, sizeof(void));
		read(db->block_api->fd, node->keys[i].data, node->keys[i].size);
	}
	for(i = 0; i < node->n; i++) {
		read(db->block_api->fd, &node->values[i].size, sizeof(node->values[i].size));
		node->values[i].data = calloc(node->values[i].size, sizeof(void));
		read(db->block_api->fd, node->values[i].data, node->values[i].size);
	}
	if (!node->leaf) {
		node->children = (int *)malloc(sizeof(*node->children)*(node->n+1));
		read(db->block_api->fd, node->children, sizeof(*node->children)*(node->n+1));
	}

	return 0;
}
int write_block(struct DB *db, struct Node *node)
{
	int i;
	lseek(db->block_api->fd, node->num_vertix*db->block_api->page_size, SEEK_SET);

	//write meta data
	write(db->block_api->fd, &node->n, sizeof(node->n));
	write(db->block_api->fd, &node->leaf, sizeof(node->leaf));

	//write keys, values and children
	for(i = 0; i < node->n; i++) {
		write(db->block_api->fd, &node->keys[i].size, sizeof(node->keys[i].size));
		write(db->block_api->fd, node->keys[i].data, node->keys[i].size);
	}
	for(i = 0; i < node->n; i++) {
		write(db->block_api->fd, &node->values[i].size, sizeof(node->values[i].size));
		write(db->block_api->fd, node->values[i].data, node->values[i].size);
	}
	if (!node->leaf)
		write(db->block_api->fd, node->children, 
			(node->n+1)*sizeof(*node->children));
	//check for max size
	return 0;
}
int clear_block(struct DB *db, int num_vertix)
{
	lseek(db->block_api->fd, num_vertix*db->block_api->page_size, SEEK_SET);
	write(db->block_api->fd, 0, db->block_api->page_size);
	return 0;
}
int fffree(struct BlockAPI *block_api)
{
	//printf("Start free for block_api\n");
	fflush(stdout);
	block_api->bitmap->free(block_api->bitmap);
	free(block_api->bitmap);
	close(block_api->fd);
	//printf("End free for block_api\n");
	fflush(stdout);
	return 0;
}
int write_bitmask(struct BlockAPI *block_api)
{
	lseek(block_api->fd, sizeof(int), SEEK_SET);
	write(block_api->fd, &block_api->bitmap->N, sizeof(block_api->bitmap->N));
	write(block_api->fd, block_api->bitmap->bitmask, 
		sizeof(*block_api->bitmap->bitmask)*block_api->bitmap->N);
	return 0;
}
int read_bitmask(struct BlockAPI *block_api)
{
	lseek(block_api->fd, sizeof(int), SEEK_SET);
	read(block_api->fd, &block_api->bitmap->N, 
		sizeof(block_api->bitmap->N));
	block_api->bitmap->bitmask = (unsigned char *)calloc(block_api->bitmap->N,
		sizeof(*block_api->bitmap->bitmask));
	read(block_api->fd, block_api->bitmap->bitmask, 
		sizeof(*block_api->bitmap->bitmask)*block_api->bitmap->N);
	return 0;
}

struct Node * create_node(struct DB *db)
{
	struct Node *node = (struct Node *)malloc(sizeof(*node));
	node->num_vertix = db->block_api->bitmap->first_empty(db->block_api->bitmap);
	db->block_api->bitmap->set(db->block_api->bitmap, node->num_vertix);
	node->close_node = &close_node;
	node->write_node = &write_node;
	return node;
}
struct Node * open_node(struct DB *db, int num_vertix)
{
	struct Node *node = (struct Node *)malloc(sizeof(*node));
	node->close_node = &close_node;
	node->write_node = &write_node;
	node->num_vertix = num_vertix;
	db->block_api->read_block(db, node);
	return node;
}
int write_node(struct DB *db, struct Node *node)
{
	return db->block_api->write_block(db, node);
}
int delete_node(struct DB *db, struct Node *node)
{
	db->block_api->clear_block(db, node->num_vertix);
	db->block_api->bitmap->unset(db->block_api->bitmap, node->num_vertix);
	db->block_api->write_bitmask(db->block_api);
	return 0;
}
int close_node(struct DB *db, struct Node *node)
{
	int i;
	for (i = 0; i < node->n; i++) {
		free(node->keys[i].data);
		free(node->values[i].data);
	} 
	free(node->keys);
	free(node->values);
	if (!node->leaf)
		free(node->children);
	return 0;
}

int find_block(struct DB *db, struct Node *node)
{
	struct BlockCache *block_cache = db->block_cache;
	char str[4];
	sprintf(str, "%d", node->num_vertix);
	if (block_cache->size)
		return 0;
	//return 1 if node in cache
	return 0;
}

int push_block(struct DB *db, struct Node *node)
{	
	struct BlockCache *block_cache = (struct BlockCache *)malloc(sizeof(*block_cache));
	if (block_cache->size + 1 >= MAX_CACHE_SIZE)
		return 1;
	return 0;
}

int pop_block(struct DB *db)
{
	return 0;
}

struct BlockCache * create_cache(void)
{
	struct BlockCache *block_cache = (struct BlockCache *)malloc(sizeof(*block_cache));
	block_cache->n_pages = 0;
	block_cache->lru = NULL;
	block_cache->size = 0;

	block_cache->all_pages = (int *)calloc(block_cache->n_pages, sizeof(*block_cache->all_pages));

	block_cache->find_block = &find_block;
	block_cache->push_block = &push_block;
	block_cache->pop_block = &pop_block;
	return block_cache;
}
