#pragma once
#include <stddef.h>
#include "block.h"

struct DB {
	/* Public API */
	/* Returns 0 on OK, -1 on Error */
	int (*close)(struct DB *db);
	int (*del)(struct DB *db, const struct DBT *key);
	/* * * * * * * * * * * * * *
	* Returns malloc'ed data into 'struct DBT *data'.
	* Caller must free data->data. 'struct DBT *data' must be alloced in
	* caller.
	* * * * * * * * * * * * * */
	int (*get)(struct DB *db, const struct DBT *key, struct DBT *data);
	int (*put)(struct DB *db, const struct DBT *key, const struct DBT *data);
	/* For future uses - sync cached pages with disk
	* int (*sync)(const struct DB *db)
	* */
	/* Private API */
	struct Disk *disk;
	struct Node *node;
	int t;
}; /* Need for supporting multiple backends (HASH/BTREE) */

struct DB * dbcreate(const char *file, struct DBC *conf);
struct DB * dbopen(const char *file, struct DBC *conf);
int db_close(struct DB *db);
int db_del(struct DB *, void *, size_t);
int db_get(struct DB *, void *, size_t, void **, size_t *);
int db_put(struct DB *, void *, size_t, void * , size_t );
struct DB * dbopen_index(const char *file, struct DBC *conf, int index);
/* For future uses - sync cached pages with disk
* int db_sync(const struct DB *db);
* */
