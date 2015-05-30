#pragma once

#include <stddef.h>
#include "block_api.h"
/* check `man dbopen` */


/* Open DB if it exists, otherwise create DB */
struct DB *dbcreate(const char *file, struct DBC *conf);

int db_close(struct DB *db);
int db_delete(struct DB *, void *, size_t);
int db_select(struct DB *, void *, size_t, void **, size_t *);
int db_insert(struct DB *, void *, size_t, void * , size_t  );

/* Sync cached pages with disk */
int db_sync(const struct DB *db);