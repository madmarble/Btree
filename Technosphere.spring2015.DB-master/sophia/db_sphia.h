#include "third_party/sophia/db/sophia.h"

struct DBT {
	void  *data;
	size_t size;
};

struct DB {
	/* Public API */
	int (*close)(struct DB *db);
	int (*delete)(struct DB *db, struct DBT *key);
	int (*select)(struct DB *db, struct DBT *key, struct DBT *data);
	int (*insert)(struct DB *db, struct DBT *key, struct DBT *data);

	/* Private API */
	void *env;
	void *db;
};

struct DBC {
	size_t db_size;
	size_t chunk_size;
};

struct DB *dbcreate(char *path, struct DBC conf);
struct DB *dbopen  (char *path, struct DBC conf);

int db_close(struct DB *db);
int db_delete(struct DB *, void *, size_t);
int db_select(struct DB *, void *, size_t, void **, size_t *);
int db_insert(struct DB *, void *, size_t, void * , size_t  );
