#include "mydb.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

void print_node(struct Node *node)
{
	printf("In node %d exist %d keys and values\n", node->num_vertix, node->n);
	if (node->leaf)
		printf("This node is also a leaf\n");
	else
		printf("This node is not a leaf\n");
	int i;
	for (i = 0; i < node->n; i++) {
		printf("Size of key is %d and value is %d\n", node->keys[i].size, 
			*(int *)node->keys[i].data);
		printf("Size of value is %d and value is %d\n", node->values[i].size, 
			*(int *)node->values[i].data);
	}

	if (!node->leaf) {
		for (i = 0; i < node->n+1; i++) {
			printf("Child %d is %d\n", i, node->children[i]);
		}
	}
	printf("\n");
	fflush(stdout);

	return;
}

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
int split(struct DB *db, int i)
{
	printf("We splitting node %d\n", db->node->num_vertix);
	struct Node *x = db->node;
	struct Node *z = db->create_node(db);
	struct Node *y = db->open_node(db, x->children[i]);

	z->leaf = y->leaf;
	print_node(x);
	print_node(y);
	print_node(z);

	z->n = db->t-1;
	z->keys = (struct DBT *)malloc(sizeof(*z->keys)*z->n);
	z->values = (struct DBT *)malloc(sizeof(*z->values)*z->n);
	if (!z->leaf)
		z->children = (int *)malloc(sizeof(*z->children)*(z->n+1));

	int j;
	for (j = 0; j < db->t-1; j++) {
		z->keys[j] = y->keys[j+db->t];
		z->values[j] = y->values[j+db->t];
	}
	if (!y->leaf) {
		for (j = 0; j < db->t; j++) {
			z->children[j] = y->children[j+db->t];
		}
	}
	print_node(z);

	y->n = db->t-1;
	print_node(y);

	for (j = x->n; j >= i+1; j--) {
		x->children[j+1] = x->children[j];
	}
	x->children[i+1] = z->num_vertix;

	for (j = x->n-1; j >= i; j--) {
		x->keys[j+1] = x->keys[j];
		x->values[j+1] = x->values[j];
	}
	x->keys[i].size = y->keys[db->t-1].size;
	x->keys[i].data = calloc(x->keys[i].size, 1);
	memcpy(x->keys[i].data, y->keys[db->t-1].data, x->keys[i].size);

	x->values[i].size = y->values[db->t-1].size;
	x->values[i].data = calloc(x->values[i].size, 1);
	memcpy(x->values[i].data, y->values[db->t-1].data, x->values[i].size);
	
	x->n++;
	print_node(x);

	db->block_api->write_block(db, z);
	z->close_node(db, z);
	db->block_api->write_block(db, y);
	y->close_node(db, y);
	db->block_api->write_block(db, x);
	return 0;
}
int insert_nonfull(struct DB *db, struct DBT *key, struct DBT *value)
{
	int i = db->node->n-1;
	print_node(db->node);
	if (db->node->leaf) {
		printf("This %d is leaf\n", db->node->num_vertix);
		if (db->node->n) {
			//printf("We have %d elements\n", db->node->n);
			fflush(stdout);
			db->node->keys = (struct DBT *)realloc(db->node->keys, (db->node->n+1)*sizeof(*db->node->keys));
			db->node->values = (struct DBT *)realloc(db->node->values, (db->node->n+1)*sizeof(*db->node->values));
			//db->node->data_keys = (struct DBT *)realloc(db->node->data_keys, (db->node->n+1)*sizeof(*db->node->data_keys));
			//db->node->data_values = (struct DBT *)realloc(db->node->data_values, (db->node->n+1)*sizeof(*db->node->data_values));
		}
		else {
			//printf("We dont have any element yet\n");
			fflush(stdout);
			db->node->keys = (struct DBT *)malloc((db->node->n+1)*sizeof(*db->node->keys));
			db->node->values = (struct DBT *)malloc((db->node->n+1)*sizeof(*db->node->values));
			//db->node->data_keys = (struct DBT *)malloc(db->node->data_keys, (db->node->n+1)*sizeof(*db->node->data_keys));
			//db->node->data_values = (struct DBT *)malloc(db->node->data_values, (db->node->n+1)*sizeof(*db->node->data_values));
		}

		while(i >= 0 &&  memcmp(db->node->keys[i].data, key->data, 
			key->size > db->node->keys[i].size ? key->size:db->node->keys[i].size) > 0) {
			db->node->keys[i+1] = db->node->keys[i];
			db->node->values[i+1] = db->node->values[i];
			i--;
		}
		//return 0;
		//printf("We insert elem %d in %d\n", *(int *)key->data, i+1);
		db->node->keys[i+1].size = key->size;
		db->node->keys[i+1].data = calloc(key->size, 1);
		memcpy(db->node->keys[i+1].data, key->data, key->size);

		db->node->values[i+1].size = value->size;
		db->node->values[i+1].data = calloc(value->size, 1);
		memcpy(db->node->values[i+1].data, value->data, value->size);

		db->node->n++;
		db->block_api->write_block(db, db->node);
		//db->close_node(db);
	} else {
		printf("This %d is not a leaf\n", db->node->num_vertix);
		while(i >= 0 &&  memcmp(db->node->keys[i].data, key->data, 
			key->size > db->node->keys[i].size ? key->size:db->node->keys[i].size) > 0) {
			i--;
		}
		//return 0;
		i++;
		struct Node *node = db->open_node(db, db->node->children[i]);
		if (node->n == 2*db->t-1) {
			node->close_node(db, node);
			db->node->keys = (struct DBT *)realloc(db->node->keys, (db->node->n+1)*sizeof(*db->node->keys));
			db->node->values = (struct DBT *)realloc(db->node->values, (db->node->n+1)*sizeof(*db->node->values));
			db->node->children = (int *)realloc(db->node->children, (db->node->n+2)*sizeof(*db->node->children));

			split(db, i);
			if (memcmp(db->node->keys[i].data, key->data, 
			key->size > db->node->keys[i].size ? key->size:db->node->keys[i].size) < 0)
				i++;
		}
		printf("We insert elem in : %d\n", db->node->children[i]);
		node = db->open_node(db, db->node->children[i]);
		struct Node *root = db->node;
		db->node = node;
		insert_nonfull(db, key, value);
		db->block_api->write_block(db, db->node);
		db->node->close_node(db, db->node);
		free(db->node);
		db->node = root;
		db->block_api->write_block(db, db->node);
	}
	return 0;
}
int insert(struct DB *db, struct DBT *key, struct DBT *value)
{
	struct Node *r = db->node;
	if (r->n == 2*db->t - 1) {
		//printf("We are splitting node\n");
		printf("Vertix %d is full, we create new vertix\n", r->num_vertix);
		fflush(stdout);
		db->node = db->create_node(db);
		struct Node *s = db->node;
		s->leaf = 0;
		s->n = 0;
		s->children = (int *)calloc((s->n+2), sizeof(*s->children));
		s->keys = (struct DBT *)malloc((s->n+1)*sizeof(*s->keys));
		s->values = (struct DBT *)malloc((s->n+1)*sizeof(*s->values));
		s->children[0] = r->num_vertix;
		r->close_node(db, r);
		free(r);
		split(db, 0);
		insert_nonfull(db, key, value);
	}
	else {
		printf("Vertix %d is not full\n", r->num_vertix);
		insert_nonfull(db, key, value);
	}
	return 0;
}
int sselect(struct DB *db, struct DBT *key, struct DBT *data)
{
	int i = 0;
	//print_node(db->node);
	while(i < db->node->n && memcmp(db->node->keys[i].data, key->data, 
			key->size > db->node->keys[i].size ? key->size:db->node->keys[i].size) < 0) {
		i++;
	}
	if (i < db->node->n &&  memcmp(db->node->keys[i].data, key->data, 
			key->size > db->node->keys[i].size ? key->size:db->node->keys[i].size) == 0) {
		data->size = db->node->values[i].size;
		data->data = calloc(data->size, 1);
		memcpy(data->data, db->node->values[i].data, data->size);
		return i;
	}
	else if (db->node->leaf) {
		return -1;
	}
	struct Node *root_node = db->node;

	//printf("This is num of next node %d\n", db->node->children[i]);
	db->node = db->open_node(db, db->node->children[i]);
	int flg = sselect(db, key, data);
	db->node->close_node(db, db->node);
	free(db->node);
	db->node = root_node;

	return flg;
}
int cclose(struct DB *db)
{
	//printf("Start db_close\n");
	fflush(stdout);
	lseek(db->block_api->fd, 0, SEEK_SET);
	write(db->block_api->fd, &db->node->num_vertix, sizeof(db->node->num_vertix));
	db->block_api->write_bitmask(db->block_api);
	db->node->close_node(db, db->node);
	db->block_api->free(db->block_api);
	free(db->block_api);
	free(db->node);
	free(db);
	//printf("End db_close\n");
	fflush(stdout);
	return 0;
}
struct DB *dbcreate(const char *file, struct DBC *conf)
{
	struct DB *db = (struct DB *)malloc(sizeof(struct DB)*1);
	db->insert = &insert;
	db->select = &sselect;
	db->close = &cclose;
	db->create_node = &create_node;
	db->open_node = &open_node;
	db->t = 4;

	db->block_api = (struct BlockAPI *)malloc(sizeof(struct BlockAPI)*1);
	db->block_api->max_size = conf->db_size;
	db->block_api->read_block = &read_block;
	db->block_api->write_block = &write_block;
	db->block_api->clear_block = &clear_block;
	db->block_api->write_bitmask = &write_bitmask;
	db->block_api->read_bitmask = &read_bitmask;
	db->block_api->free = &fffree;
	db->block_api->page_size = conf->page_size;

	db->block_api->bitmap = (struct bitmap *)malloc(sizeof(*db->block_api->bitmap)*1);
	db->block_api->bitmap->init = &init;
	db->block_api->bitmap->free = &ffree;
	db->block_api->bitmap->set = &set;
	db->block_api->bitmap->unset = &unset;
	db->block_api->bitmap->first_empty = &first_empty;
	db->block_api->bitmap->show = &show;

	printf("Attempt to open file %s\n", file);
	fflush(stdout);
	db->block_api->fd = open(file, O_RDWR);
	if (db->block_api->fd == -1) {
		printf("File doesnt exist, we are creating it now\n");
		//create file
		db->block_api->fd = open(file, O_RDWR | O_CREAT);

		//making file size conf->db_size
		//lseek(db->block_api->fd, conf->db_size, SEEK_SET);
		//lseek(db->block_api->fd, 0, SEEK_SET);

		//setting bitmap
		db->block_api->bitmap->init(db->block_api->bitmap, conf->db_size/conf->page_size);
		char rest;
		if ((db->block_api->bitmap->N + 2*sizeof(int))%db->block_api->page_size) rest = 1;
		else rest = 0;
		int n = (db->block_api->bitmap->N + 2*sizeof(int))/db->block_api->page_size + rest;
		int i;
		for (i = 0; i < n; i++) {
			db->block_api->bitmap->set(db->block_api->bitmap, i);
		}

		//making node
		db->node = db->create_node(db);
		db->node->n = 0;
		db->node->leaf = 1;

		lseek(db->block_api->fd, sizeof(int), SEEK_SET);
		write(db->block_api->fd, &db->node->num_vertix, sizeof(db->node->num_vertix));
		db->block_api->write_block(db, db->node);
	} 
	else {
		printf("File already created, we are reading from it\n");
		int num_vertix;
		read(db->block_api->fd, &num_vertix, sizeof(num_vertix));
		db->block_api->read_bitmask(db->block_api);
		db->node = db->open_node(db, num_vertix);
	}

	return db;
}
void print_status(struct DB *db, struct Node *node)
{
	printf("In node %d exist %d keys and values\n", node->num_vertix, node->n);
	if (node->leaf)
		printf("This node is also a leaf\n");
	else
		printf("This node is not a leaf\n");
	int i;
	for (i = 0; i < node->n; i++) {
		printf("Size of key is %d and value is %d\n", node->keys[i].size, 
			*(int *)node->keys[i].data);
		printf("Size of value is %d and value is %d\n", node->values[i].size, 
			*(int *)node->values[i].data);
	}

	if (!node->leaf) {
		for (i = 0; i < node->n+1; i++) {
			printf("Child %d is %d\n", i, node->children[i]);
		}
	}
	printf("\n");
	fflush(stdout);

	if (!node->leaf) {
		for (i = 0; i < node->n+1; i++) {
			struct Node *new_node = db->open_node(db, node->children[i]);
			print_status(db, new_node);
			new_node->close_node(db, new_node);
			free(new_node);
		}
	}
	return;
}