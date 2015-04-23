#include "db_sphia.h"

#include <assert.h>
#include <stdio.h>
/* struct DB API */

int sph_close(struct DB *db) {
	if (db->env) {
		sp_destroy(db->env);
		db->env = NULL;
	}
	if (db->db) {
		sp_destroy(db->db);
		db->db = NULL;
	}
	return 0;
}

int sph_delete(struct DB *db, struct DBT *key) {
	int rc = sp_delete(db->db, key->data, key->size);
	if (rc == -1) {
		printf("error: %s\n", sp_error(db->env));
		assert(0);
	}
	return 0;
}

int sph_select(struct DB *db, struct DBT *key, struct DBT *data) {
	int rc = sp_get(db->db, key->data, key->size, &data->data, &data->size);
	if (rc == -1) {
		printf("error: %s\n", sp_error(db->env));
		assert(0);
	}
	return 0;
}

int sph_insert(struct DB *db, struct DBT *key, struct DBT *data) {
	int rc = sp_set(db->db, key->data, key->size, data->data, data->size);
	if (rc == -1) {
		printf("error: %s\n", sp_error(db->env));
		assert(0);
	}
	return 0;
}

/* * * * * * * * *
 * dbopen and dbcreate for sophia must have DBC
 * (sophia doesn't save page size)
 * * * * * * * * */

struct DB *dbcreate(char *path, struct DBC conf) {
	struct DB *db = (struct DB *)calloc(1, sizeof(struct DB));
	int rc = 0;
	assert(db);
	db->env = sp_env();
	assert(db->env);
	rc = sp_ctl(db->env, SPDIR, SPO_CREAT|SPO_RDWR, path);
	if (rc == -1) {
		printf("error: %s\n", sp_error(db->env));
		assert(0);
	}
	db->db = sp_open(db->env);
	if (!db->db) {
		printf("error: %s\n", sp_error(db->env));
		assert(0);
	}
	db->select   = sph_select;
	db->insert   = sph_insert;
	db->delete   = sph_delete;
	db->close = sph_close;
	return db;
}

/* Testing API for calling from Python */

int db_close(struct DB *db) {
	return db->close(db);
}

int db_delete(struct DB *db, void *key, size_t key_len) {
	struct DBT keyt = {
		.data = key,
		.size = key_len
	};
	return db->delete(db, &keyt);
}

int db_select(struct DB *db, void *key, size_t key_len,
		void **val, size_t *val_len) {
	struct DBT keyt = {
		.data = key,
		.size = key_len
	};
	struct DBT valt = {0, 0};
	int rc = db->select(db, &keyt, &valt);
	*val = valt.data;
	*val_len = valt.size;
	return rc;
}

int db_insert(struct DB *db, void *key, size_t key_len,
		void *val, size_t val_len) {
	struct DBT keyt = {
		.data = key,
		.size = key_len
	};
	struct DBT valt = {
		.data = val,
		.size = val_len
	};
	return db->insert(db, &keyt, &valt);
}
