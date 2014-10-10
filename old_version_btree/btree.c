#include "btree.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
int t;
char **file_names;
int size_of_file_names;
int size_of_malloced_file_names;
int size_of_new_string = 100;
struct DBC conf_created;

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
struct DB *dbcreate(const char *file, const struct DBC conf)
{
	int fd = open(file, O_CREAT | O_RDWR, S_IWRITE | S_IREAD);
	struct DB *res = (struct DB *)malloc(1 * sizeof(struct DB));
	res->fd = fd;
	res->n = 0;
	res->leaf = 1;
	return res;
}
struct DB *dbopen (const char *file, const struct DBC conf)
{
	int fd = open(file, O_RDWR, 0777);
	if (fd == -1) return NULL;

	struct DB *res = (struct DB *)malloc(1 * sizeof(struct DB));
	res->fd = fd;

	//reading num and references to children
	read(fd, &(res->n), sizeof(int));
	read(fd, &(res->leaf), sizeof(char));
	res->neighbours = (int *)malloc(sizeof(int) * (res->n + 1));
	int i = 0;
	for (i = 0; i < res->n + 1; i++) {
		read(fd, &res->neighbours[i], sizeof(int *));
	}
	
	//reading keys and value
	lseek(fd, 0, SEEK_END);
	res->keys = (struct DBT *)malloc(sizeof(struct DBT) * (res->n));
	for (i = 0; i < res->n; i++) {
		read(fd, &((res->keys[i]).size), sizeof(char));
		read(fd, &((res->keys[i]).data), res->keys->size);
	}
	res->values = (struct DBT *)malloc(sizeof(struct DBT) * (res->n));
	for (i = 0; i < res->n; i++) {
		read(fd, &(res->values[i]).size, sizeof(char));
		read(fd, &(res->values[i]).data, res->keys->size);
	}
	lseek(fd, 0, SEEK_SET);
	return res;
}
void disk_write(struct DB *x)
{
	printf("disk_write Starting\n");
	int i;
	write(x->fd, &x->n, sizeof(int));
	write(x->fd, &x->leaf, sizeof(char));
	for (i = 0; i < x->n + 1; i++) {
		write(x->fd, &x->neighbours[i], sizeof(int));
	}

	lseek(x->fd, 0, SEEK_END);
	for (i = 0; i < x->n; i++) {
		write(x->fd, &((x->keys[i]).size), sizeof(char));
		write(x->fd, ((x->keys[i]).data), x->keys->size);
		printf("%d\n", *(int *)x->keys[i].data);
	}
	//printf("\n");
	for (i = 0; i < x->n; i++) {
		write(x->fd, &(x->values[i]).size, sizeof(char));
		write(x->fd, &(x->values[i]).data, x->keys->size);
	}
	lseek(x->fd, 0, SEEK_SET);
	return;
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
void b_tree_split_child(struct DB *x, int i)
{
	printf("b_tree_split_child Starting\n");
	if (size_of_file_names == size_of_malloced_file_names) {
		size_of_malloced_file_names *= 2;
		file_names = (char **)realloc(file_names, size_of_malloced_file_names * sizeof(char *));
	}
	
	file_names[size_of_file_names] = (char *)malloc(sizeof(char) * size_of_new_string);
	sprintf(file_names[size_of_file_names], "%s", file_names[x->own_tag]);
	sprintf(file_names[size_of_file_names], "/%d", size_of_file_names);

	struct DB *z = dbcreate(file_names[size_of_file_names], conf_created);
	z->own_tag = size_of_file_names;
	size_of_file_names++;
	struct DB *y = dbopen(file_names[i], conf_created);
	z->leaf = y->leaf;
	z->n = t - 1;
	z->keys = (struct DBT *)malloc(sizeof(struct DBT) * (z->n));
	z->values = (struct DBT *)malloc(sizeof(struct DBT) * (z->n));
	z->neighbours = (int *)malloc(sizeof(int) * (z->n + 1));

	int j;
	for (j = 0; j < t - 1; j++) {
		z->keys[j] = y->keys[j + t];
		z->values[j] = y->values[j + t];
	}
	if (!y->leaf) {
		for (j = 0; j < t; j++) {
			z->neighbours[j] = y->neighbours[j + t];
		}
	}
	y->n = t - 1;
	for (j = x->n; j >= i + 1; j--) {
		x->neighbours[j + 1] = x->neighbours[j];
	}
	x->neighbours[i + 1] = size_of_file_names - 1;
	for(j = x->n - 1; j >= i; j--) {
		x->keys[j + 1] = x->keys[j];
	}
	x->keys[i] = y->keys[t - 1];
	x->n++;
	disk_write(y);
	disk_write(z);
	disk_write(x);
	//freedom(y);
	//freedom(z);
	//freedom(x);
	return;
}
//still 
void b_tree_insert_nonfull(struct DB *x, struct DBT *key)
{
	printf("b_tree_insert_nonfull Starting\n");
	x->keys = (struct DBT *)realloc(x->keys, sizeof(struct DBT) * (x->n + 1));
	int i = x->n;
	if (x->leaf) {
		printf("%d\n", i);
		while (i >= 0 && memcmp(key->data, &x->keys[i].data, (x->keys[i].size < key->size ? x->keys[i].size : key->size)) < 0)
		{
			printf("comparing Starting\n");
			x->keys[i+1] = x->keys[i];
			i--;
		}
		printf("%d\n", *(int *)key->data);
		printf("%d\n", i + 1);
		x->keys[i+1] = *key;
		x->n++;
		disk_write(x);
		//freedom(x);
	}
	return;
}
void insert(struct DB *root, struct DBT *key)
{
	printf("insert Starting\n");
	struct DB *tmp = root;
	if (root->n == 2 * t - 1) {
		if (size_of_file_names == size_of_malloced_file_names) {
			size_of_malloced_file_names *= 2;
			file_names = (char **)realloc(file_names, size_of_malloced_file_names * sizeof(char *));
		}
		
		file_names[size_of_file_names] = (char *)malloc(sizeof(char) * size_of_new_string);
		sprintf(file_names[size_of_file_names], "%s", file_names[root->own_tag]);
		sprintf(file_names[size_of_file_names], "/%d", size_of_file_names);

		struct DB *z = dbcreate(file_names[size_of_file_names], conf_created);
		size_of_file_names++;
		root = z;
		z->leaf = 0;
		z->neighbours = (int *)malloc(sizeof(int) * (z->n + 1));
		z->neighbours[0] = tmp->own_tag;
		b_tree_split_child(z, 0);
		b_tree_insert_nonfull(z, key);
	} else b_tree_insert_nonfull(root, key);
	return;
}

int main(int argc, char **argv)
{
	printf("Starting\n");
	file_names = (char **)malloc(sizeof(char *) * (argc - 1));
	size_of_file_names = argc - 1;
	size_of_malloced_file_names = argc - 1;
	conf_created.db_size = 4096;
	conf_created.chunk_size = 512 * 1024 * 1024;
	scanf("%d", &t);
	struct DB *res = NULL;
	int i;
	for (i = 1; i < argc; i++) {
		file_names[i - 1] = argv[i];
		res = dbopen(argv[i], conf_created);
		if (!res){
			printf("Create file and this is strange as i think\n");
			res = dbcreate(argv[i], conf_created);
			printf("NUM for res %d\n", res->n);
		} else {
			printf("Open file %s\n", argv[i]);
		}
		res->own_tag = i - 1;
	}
	int k = 10;
	struct DBT *key = (struct DBT *)malloc(sizeof(struct DBT));
	key->data = &k;
	key->size = sizeof(int);
	insert(res, key);
	return 0;
}