#include <stddef.h>
struct DBT {
	void *data;
	size_t size;
};
struct DB {
	/* Public API */
	/* Returns 0 on OK, -1 on Error */
	int (*close)(struct DB *db);
	int (*del)(const struct DB *db, const struct DBT *key);
	/* * * * * * * * * * * * * *
	* Returns malloc'ed data into 'struct DBT *data'.
	* Caller must free data->data. 'struct DBT *data' must be alloced in
	* caller.
	* * * * * * * * * * * * * */
	int (*get)(const struct DB *db, const struct DBT *key, struct DBT *data);
	int (*put)(const struct DB *db, const struct DBT *key, const struct DBT *data);
	/* For future uses - sync cached pages with disk
	* int (*sync)(const struct DB *db)
	* */

	char leaf;
	int n;
	int own_tag;
	int *neighbours;
	struct DBT *keys;
	struct DBT *values;

	/* Private API */
	/* ... */
}; /* Need for supporting multiple backends (HASH/BTREE) */
struct DBC {
	size_t db_size;
	size_t chunk_size;
	int t;
	char *exist_or_not;
	int start_offset;
	int root_offset;
	int fd;
	int count_blocks;
	int first_empty;
	/* For future uses - maximum cached memory size
	* 16MB by default
	* size_t mem_size; */
};
/* don't store metadata in the file */
void dbcreate(const char *file, struct DBC *conf);
int dbopen(const char *file, struct DBC *conf);
int db_close(struct DB *db);
int db_del(const struct DB *, void *, size_t);
int db_get(const struct DB *, void *, size_t, void **, size_t *);
int db_put(const struct DB *, void *, size_t, void * , size_t );
/* For future uses - sync cached pages with disk
* int db_sync(const struct DB *db);
* */
