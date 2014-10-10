#include "block.h"

int db_close(struct DB *db) {
	return db->close(db);
}
int db_del(const struct DB *db, void *key, size_t key_len) {
	struct DBT keyt = {
		.data = key,
		.size = key_len
	};
	return db->del(db, &keyt);
}
int db_get(const struct DB *db, void *key, size_t key_len, void **val, size_t *val_len) {
	struct DBT keyt = {
		.data = key,
		.size = key_len
	};
	struct DBT valt = {0, 0};
	int rc = db->get(db, &keyt, &valt);
	*val = valt.data;
	*val_len = valt.size;
	return rc;
}
int db_put(const struct DB *db, void *key, size_t key_len, void *val, size_t val_len) {
	struct DBT keyt = {
		.data = key,
		.size = key_len
	};
	struct DBT valt = {
		.data = val,
		.size = val_len
	};
	return db->put(db, &keyt, &valt);
}
void freedom(struct DB *x)
{
	int i;
	for(i = 0; i < x->n; i++) {
		free(x->values[i].data);
		free(x->keys[i].data);
	}
	free(x->values);
	free(x->keys);
	free(x->neighbours);
	free(x);
	return;
}

void b_tree_split_child(struct DB *x, struct DB *y, struct DBC *conf)
{
	printf("B_tree_split_child starting\n");
	struct DB *z = create_block(conf);
	z->leaf = y->leaf;
	z->n = conf->t - 1;
	z->keys = (struct DBT *)malloc(sizeof(struct DBT) * (z->n));
	z->values = (struct DBT *)malloc(sizeof(struct DBT) * (z->n));
	z->neighbours = (int *)malloc(sizeof(int) * (z->n + 1));

	int j;
	for (j = 0; j < conf->t - 1; j++) {
		z->keys[j] = y->keys[j + conf->t];
		z->values[j] = y->values[j + conf->t];
	}
	if (!y->leaf) {
		for (j = 0; j < conf->t; j++) {
			z->neighbours[j] = y->neighbours[j + conf->t];
		}
	}
	y->n = conf->t - 1;
	for (j = x->n; j >= x->n/2 + x->n%2; j--) {
		x->neighbours[j + 1] = x->neighbours[j];
	}
	x->neighbours[j + 1] = z->own_tag;
	for(j = x->n - 1; j >= x->n/2 + x->n%2; j--) {
		x->keys[j + 1] = x->keys[j];
	}
	x->keys[j] = y->keys[conf->t - 1];
	x->n++;
	write_block(y, conf);
	write_block(z, conf);
	write_block(x, conf);
	freedom(z);
	return;
}
void b_tree_insert_nonfull(struct DB *x, struct DBT *key, struct DBC *conf)
{
	printf("B_tree_insert_nonfull starting\n");
	x->keys = (struct DBT *)realloc(x->keys, sizeof(struct DBT) * (x->n + 1));
	int i = x->n;
	if (x->leaf) {
		while (i >= 0 && memcmp(key->data, &x->keys[i].data, (x->keys[i].size < key->size ? x->keys[i].size : key->size)) < 0)
		{
			//printf("Comparing starting\n");
			x->keys[i+1] = x->keys[i];
			i--;
		}
		x->keys[i+1] = *key;
		x->n++;
		write_block(x, conf);
	}
	printf("B_tree_insert_nonfull ended\n");
	return;
}
void insert(struct DBT *key, struct DBC *conf)
{
	printf("Insert starting\n");
	struct DB *root = read_block(conf, conf->root_offset);
	if (root == NULL) root = create_block(conf);
	if (root->n == 2 * conf->t - 1) {
		struct DB *z = create_block(conf);
		if (z == NULL) {
			printf("Cant create block\n");
			return;
		}
		conf->root_offset = z->own_tag;
		z->neighbours = (int *)malloc(sizeof(int) * (z->n + 1));
		z->neighbours[0] = root->own_tag;
		struct DB *y = read_block(conf, conf->root_offset);
		b_tree_split_child(z, y, conf);
		b_tree_insert_nonfull(z, key, conf);
		freedom(z);
		freedom(y);
	} else b_tree_insert_nonfull(root, key, conf);
	//freedom(root);
	printf("Insert ended\n");
	return;
}

int main(int argc, char **argv)
{
	printf("Starting programm\n");

	printf("Start creating settings\n");
	struct DBC *conf = (struct DBC *)calloc(1, sizeof(struct DBC));
	if (dbopen(argv[1], conf)) {
		dbcreate(argv[1], conf);
	}

	lseek(conf->fd, conf->start_offset * conf->chunk_size, SEEK_SET);
	printf("Start reading blocks from file %s", argv[1]);
	int i;
	for(i = conf->start_offset; i < conf->count_blocks; i++) {
		struct DB * res = read_block(conf, i);
		if (res == NULL) {
			conf->exist_or_not[i] = 0;
			printf("Block number %d empty\n", i);
			if (conf->first_empty == -1) conf->first_empty = i;
		} else {
			conf->exist_or_not[i] = 1;
			printf("Block number %d busy\n", i);
		}
	}
	if (conf->first_empty == -1) {
		printf("Free space ended!\n");
		return -1;
	}
	int k = 10;
	struct DBT *key = (struct DBT *)malloc(sizeof(struct DBT));
	key->data = &k;
	key->size = sizeof(int);
	insert(key, conf);
	return 0;
}