#pragma once
#include <stddef.h>
struct DBT {
	void *data;
	size_t size;
};
struct DBC {
	size_t db_size;
	size_t chunk_size;
	int t;
	char *exist_or_not;
	int start_offset;
	int root_offset;
	int count_blocks;
	int fd;
	int first_empty;
	/* For future uses - maximum cached memory size
	* 16MB by default
	* size_t mem_size; */
};
struct DB {
	/* Public API */
	/* Returns 0 on OK, -1 on Error */
	int (*close)(struct DB *db);
	int (*del)(struct DB *root, const struct DBT *key);
	/* * * * * * * * * * * * * *
	* Returns malloc'ed data into 'struct DBT *data'.
	* Caller must free data->data. 'struct DBT *data' must be alloced in
	* caller.
	* * * * * * * * * * * * * */
	int (*get)(struct DB *root, const struct DBT *key, struct DBT *data);
	int (*put)(struct DB *root, const struct DBT *key, const struct DBT *data);
	/* For future uses - sync cached pages with disk
	* int (*sync)(const struct DB *db)
	* */

	char leaf;
	int n;
	int own_tag;
	int *neighbours;
	struct DBT *keys;
	struct DBT *values;
	struct DBC *conf;
	/* Private API */
	/* ... */
}; /* Need for supporting multiple backends (HASH/BTREE) */

struct DB * dbcreate(struct DBC *conf);
struct DB * dbopen(const char *file, struct DBC *conf);
int db_close(struct DB *db);/*
int db_del(struct DB *, void *, size_t);*/
int db_get(struct DB *, void *, size_t, void **, size_t *);
int db_put(struct DB *, void *, size_t, void * , size_t );
void write_conf(struct DBC *conf);
/* For future uses - sync cached pages with disk
* int db_sync(const struct DB *db);
* */
