#pragma once

#include <stddef.h>

struct DBT {
	void  *data;
	char size;
};

struct DB;
enum { dirty, clean } status;

struct Node
{
	int num_vertix; // number of active vertix
	int n;
	int leaf;
	struct DBT *keys;
	struct DBT *values;
	int *children;
	char write;

	struct Node *lru_next;
	struct Node *lru_prev;
	//status stat;

	int (*close_node)(struct DB *db, struct Node *node);
	int (*write_node)(struct DB *db, struct Node *node);
};

struct BlockCache
{
	size_t n_pages;
	struct Node *lru;
	int *all_pages;
	size_t size;

	int (*find_block)(struct DB *db, struct Node *node);
	int (*push_block)(struct DB *db, struct Node *node);
	int (*pop_block)(struct DB *db);
};

struct DB {
	/* Public API */
	/* Returns 0 on OK, -1 on Error */
	int (*close)(struct DB *db);
	int (*delete)(struct DB *db, struct Node *node, struct DBT *key);
	int (*insert)(struct DB *db, struct NOde *node, struct DBT *key, struct DBT *data);
	/* * * * * * * * * * * * * *
	 * Returns malloc'ed data into 'struct DBT *data'.
	 * Caller must free data->data. 'struct DBT *data' must be alloced in
	 * caller.
	 * * * * * * * * * * * * * */
	int (*select)(struct DB *db, struct Node *node, struct DBT *key, struct DBT *data);
	/* Sync cached pages with disk
	 * */
	int (*sync)(struct DB *db);
	/* For future uses - sync cached pages with disk
	 * int (*sync)(const struct DB *db)
	 * */
	/* Private API */
	struct BlockCache *block_cache;
	struct BlockAPI *block_api;
	struct Node *node;
	struct Node * (*create_node)(struct DB *db);
	struct Node * (*open_node)(struct DB *db, int num_vertix);
	struct BlockCache * (*create_cache)(void);
	int t;
	int flag;
}; /* Need for supporting multiple backends (HASH/BTREE) */

struct DBC {
	/* Maximum on-disk file size
	 * 512MB by default
	 * */
	size_t db_size;
	/* Page (node/data) size
	 * 4KB by default
	 * */
	size_t page_size;
	/* Maximum cached memory size
	 * 16MB by default
	 * */
	size_t cache_size;
};

struct BlockData
{
	int offset;
	int size;
};

struct bitmap {
	unsigned char *bitmask;
	int N;
	int (*init)(struct bitmap *bitmap, int n); // fill bitmask by zero
	int (*free)(struct bitmap *bitmap); // free bitmask
	int (*set)(struct bitmap *bitmap, int i); // set i bit in 1
	int (*unset)(struct bitmap *bitmap, int i); // set i bit in 0
	int (*first_empty)(struct bitmap *bitmap); // return number of first empty block
	int (*show)(struct bitmap *bitmap);
};

struct BlockAPI {
	int fd;
	int page_size;
	int max_size;
	struct bitmap *bitmap;
	int (*read_block)(struct DB *db, struct Node *node);
	int (*write_block)(struct DB *db, struct Node *node);
	int (*clear_block)(struct DB *db, int num_vertix);
	int (*write_bitmask)(struct BlockAPI *block_api); // write bitmask in file
	int (*read_bitmask)(struct BlockAPI *block_api); // read bitmask from file
	int (*free)(struct BlockAPI *block_api);
};

int init(struct bitmap *bitmap, int n);
int ffree(struct bitmap *bitmap);
int set(struct bitmap *bitmap, int i);
int unset(struct bitmap *bitmap, int i);
int first_empty(struct bitmap *bitmap);
int show(struct bitmap *bitmap);

int fffree(struct BlockAPI *block_api);
int write_bitmask(struct BlockAPI *block_api);
int read_bitmask(struct BlockAPI *block_api);

int read_block(struct DB *db, struct Node *node);
int write_block(struct DB *db, struct Node *node);
int clear_block(struct DB *db, int num_vertix);

int insert(struct DB *db, struct Node *node, struct DBT *key, struct DBT *value);
int sselect(struct DB *db, struct Node *node, struct DBT *key, struct DBT *data);
int cclose(struct DB *db);
int ddelete(struct DB *db, struct Node* node, struct DBT *key);

struct Node * create_node(struct DB *db);
struct Node * open_node(struct DB *db, int num_vertix);
int close_node(struct DB *db, struct Node *node);
int write_node(struct DB *db, struct Node *node);

struct BlockCache * create_cache(void);
int push_block(struct DB *db, struct Node *node);
int find_block(struct DB *db, struct Node *node);
