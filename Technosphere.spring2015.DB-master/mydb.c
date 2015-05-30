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
	fflush(stdout);
	if (node->leaf) {
		printf("This node is also a leaf\n");
		fflush(stdout);
	}
	else {
		printf("This node is not a leaf\n");
		fflush(stdout);
	}
	int i;
	for (i = 0; i < node->n; i++) {
		printf("Size of key is %d and value is %s\n", node->keys[i].size, 
			(char *)node->keys[i].data);
		fflush(stdout);
		printf("Size of value is %d and value is %s\n", node->values[i].size, 
			(char *)node->values[i].data);
		fflush(stdout);
	}

	if (!node->leaf) {
		for (i = 0; i < node->n+1; i++) {
			printf("Child %d is %d\n", i, node->children[i]);
			fflush(stdout);
		}
	}
	printf("\n");
	fflush(stdout);

	return;
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
		printf("Size of key is %d and value is %s\n", node->keys[i].size, 
			(char *)node->keys[i].data);
		printf("Size of value is %d and value is %s\n", node->values[i].size, 
			(char *)node->values[i].data);
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
int db_close(struct DB *db) {
	return db->close(db);
}

int db_delete(struct DB *db, void *key, size_t key_len) {
	struct DBT keyt = {
		.data = key,
		.size = key_len
	};
	return db->delete(db, db->node, &keyt);
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
	//printf("We splitting node %d\n", db->node->num_vertix);
	struct Node *x = db->node;
	struct Node *z = db->create_node(db);
	struct Node *y = db->open_node(db, x->children[i]);

	z->leaf = y->leaf;

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

	y->n = db->t-1;

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

	z->write_node(db, z);
	z->close_node(db, z);

	y->write_node(db, y);
	y->close_node(db, y);

	x->write_node(db, x);
	return 0;
}

int new_memcmp(struct DBT t1, struct DBT t2)
{
	int res;
	if (t1.size > t2.size)
		res = 1;
	else if (t2.size > t1.size)
		res = -1;
	else
		res = memcmp(t1.data, t2.data, t1.size);
	return res;
}

int insert_nonfull(struct DB *db, struct DBT *key, struct DBT *value)
{
	int i = db->node->n-1;
	//print_node(db->node);
	if (db->node->leaf) {
		if (i > 0) {
			while(i >= 0 &&  new_memcmp(db->node->keys[i], *key) > 0) {
				i--;
			}
			if (new_memcmp(db->node->keys[i], *key) == 0) {
				db->node->keys[i].size = key->size;
				free(db->node->keys[i].data);
				db->node->keys[i].data = calloc(db->node->keys[i].size, 1);
				memcpy(db->node->keys[i].data, key->data, db->node->keys[i].size);

				db->node->values[i].size = value->size;
				free(db->node->values[i].data);
				db->node->values[i].data = calloc(db->node->values[i].size, 1);
				memcpy(db->node->values[i].data, value->data, db->node->values[i].size);
				db->block_api->write_block(db, db->node);
				return 0;
			}
		}
		i = db->node->n-1;
		if (db->node->n) {
			db->node->keys = (struct DBT *)realloc(db->node->keys, (db->node->n+1)*sizeof(*db->node->keys));
			db->node->values = (struct DBT *)realloc(db->node->values, (db->node->n+1)*sizeof(*db->node->values));
		}
		else {
			db->node->keys = (struct DBT *)malloc((db->node->n+1)*sizeof(*db->node->keys));
			db->node->values = (struct DBT *)malloc((db->node->n+1)*sizeof(*db->node->values));
		}

		while(i >= 0 &&  new_memcmp(db->node->keys[i], *key) > 0) {
			db->node->keys[i+1] = db->node->keys[i];
			db->node->values[i+1] = db->node->values[i];
			i--;
		}
		//printf("We insert elem %d in %d\n", *(int *)key->data, i+1);
		
		db->node->keys[i+1].size = key->size;
		db->node->keys[i+1].data = calloc(key->size, 1);
		memcpy(db->node->keys[i+1].data, key->data, key->size);

		db->node->values[i+1].size = value->size;
		db->node->values[i+1].data = calloc(value->size, 1);
		memcpy(db->node->values[i+1].data, value->data, value->size);

		db->node->n++;
		db->node->write_node(db, db->node);
	} else {
		while(i >= 0 && new_memcmp(db->node->keys[i], *key) > 0) {
			i--;
		}
		if (i >= 0 && new_memcmp(db->node->keys[i], *key) == 0) {
			db->node->keys[i].size = key->size;
			free(db->node->keys[i].data);
			db->node->keys[i].data = calloc(db->node->keys[i].size, 1);
			memcpy(db->node->keys[i].data, key->data, db->node->keys[i].size);

			db->node->values[i].size = value->size;
			free(db->node->values[i].data);
			db->node->values[i].data = calloc(db->node->values[i].size, 1);
			memcpy(db->node->values[i].data, value->data, db->node->values[i].size);
			db->block_api->write_block(db, db->node);
			return 0;
		}
		i++;
		struct Node *node = db->open_node(db, db->node->children[i]);
		if (node->n == 2*db->t-1) {
			node->close_node(db, node);
			db->node->keys = (struct DBT *)realloc(db->node->keys, (db->node->n+1)*sizeof(*db->node->keys));
			db->node->values = (struct DBT *)realloc(db->node->values, (db->node->n+1)*sizeof(*db->node->values));
			db->node->children = (int *)realloc(db->node->children, (db->node->n+2)*sizeof(*db->node->children));

			split(db, i);
			if (new_memcmp(db->node->keys[i], *key) < 0)
				i++;
			else if (new_memcmp(db->node->keys[i], *key) == 0) {
				db->node->keys[i].size = key->size;
				free(db->node->keys[i].data);
				db->node->keys[i].data = calloc(db->node->keys[i].size, 1);
				memcpy(db->node->keys[i].data, key->data, db->node->keys[i].size);

				db->node->values[i].size = value->size;
				free(db->node->values[i].data);
				db->node->values[i].data = calloc(db->node->values[i].size, 1);
				memcpy(db->node->values[i].data, value->data, db->node->values[i].size);
				db->block_api->write_block(db, db->node);
				return 0;
			}
		}
		node = db->open_node(db, db->node->children[i]);
		struct Node *root = db->node;
		db->node = node;
		db->insert(db, key, value);
		db->node->write_node(db, db->node);
		db->node->close_node(db, db->node);
		free(db->node);
		db->node = root;
		db->node->write_node(db, db->node);
	}
	return 0;
}

int insert(struct DB *db, struct DBT *key, struct DBT *value)
{
	struct Node *r = db->node;
	if (r->n == 2*db->t - 1) {
		//printf("We are splitting node\n");
		//printf("Vertix %d is full, we create new vertix\n", r->num_vertix);
		//fflush(stdout);
		db->node = db->create_node(db);
		struct Node *s = db->node;
		s->leaf = 0;
		s->n = 0;
		s->children = (int *)calloc((s->n+2), sizeof(*s->children));
		s->keys = (struct DBT *)malloc((s->n+1)*sizeof(*s->keys));
		s->values = (struct DBT *)malloc((s->n+1)*sizeof(*s->values));
		s->children[0] = r->num_vertix;
		r->write_node(db, r);
		r->close_node(db, r);
		free(r);
		split(db, 0);
		insert_nonfull(db, key, value);
	}
	else {
		//printf("Vertix %d is not full\n", r->num_vertix);
		//fflush(stdout);
		insert_nonfull(db, key, value);
	}
	return 0;
}

int sselect(struct DB *db, struct DBT *key, struct DBT *data)
{
	if (db->node->n == 0)
		return 0;
	int i = 0;
	int flg = 1;
	struct Node *node = db->node;
	int first = 0;
	while(flg) {
		while(i < node->n && new_memcmp(node->keys[i], *key) < 0) {
			i++;
		}
		if (i < node->n &&  new_memcmp(node->keys[i], *key) == 0) {
			data->size = node->values[i].size;
			data->data = calloc(data->size, 1);
			memcpy(data->data, node->values[i].data, data->size);
			if (first) {
				node->close_node(db, node);
				free(node);
			}
			return i;
		}
		else if (node->leaf) {
			if (first) {
				node->close_node(db, node);
				free(node);
			}
			data->data = NULL;
			data->size = 0;
			//printf("can not find elem %s\n", (char *)key->data);
			return -1;
		}
		int child = node->children[i];
		if (first) {
			node->close_node(db, node);
		}
		node = db->open_node(db, child);
		first = 1;
		i = 0;
	}
	return flg;
}

int make_realloc(struct Node *node)
{
	node->keys = (struct DBT *)realloc(node->keys, node->n*sizeof(*node->keys));
	node->values = (struct DBT *)realloc(node->values, node->n*sizeof(*node->values));
	if (!node->leaf)
		node->children = (int *)realloc(node->children, (node->n+1)*sizeof(*node->children));
	return 0;
}

int delete_key(struct Node *node, int index)
{
	//printf("In delete_key\n");
	//fflush(stdout);
	free(node->keys[index].data);
	free(node->values[index].data);
	int i = index;
	while (i < node->n-1) {
		node->keys[i] = node->keys[i+1];
		node->values[i] = node->values[i+1];
		i++;
	}

	if (!node->leaf) {
		i = index+1;
		while (i < node->n) {
			node->children[i] = node->children[i+1];
			i++;
		}
	}
	node->n--;
	make_realloc(node);
	return 0;
}

int find_key(struct Node *node, struct DBT *key)
{
	int i = node->n-1;
	while(i >= 0 && new_memcmp(node->keys[i], *key)) {
		i--;
	}
	return i;
}

struct DBT deep_copy(struct DBT resource)
{
	struct DBT result;
	result.size = resource.size;
	result.data = calloc(result.size, 1);
	memcpy(result.data, resource.data, result.size);
	return result;
}

struct DBT * find_previous_key(struct DB *db, struct Node *node, struct DBT *key)
{
	if (node->leaf) {
		struct DBT *previous = (struct DBT *)malloc(2*sizeof(*previous));
		previous[0] = deep_copy(node->keys[node->n-1]);
		previous[1] = deep_copy(node->values[node->n-1]);
		return previous;
	}
	return find_previous_key(db, db->open_node(db, node->children[node->n]), key);
}

struct DBT * find_next_key(struct DB *db, struct Node *node, struct DBT *key)
{
	if (node->leaf) {
		struct DBT *previous = (struct DBT *)malloc(2*sizeof(*previous));
		previous[0] = deep_copy(node->keys[0]);
		previous[1] = deep_copy(node->values[0]);
		return previous;
	}
	return find_next_key(db, db->open_node(db, node->children[0]), key);
}

int ddelete(struct DB *db, struct Node *node, struct DBT *key)
{
	//printf("This is our key : %d %s\n", key->size, (char *)key->data);
	//fflush(stdout);

	int i = find_key(node, key);
	if (i >= 0 && node->leaf) {
		//printf("Leaf and we find element\n");
		//fflush(stdout);
		delete_key(node, i);
		node->write_node(db, node);
		return 0;
	}

	if (i < 0 && node->leaf) {
		//printf("Can not find elem in btree\n");
		//fflush(stdout);
		db->node->write_node(db, db->node);
		node->write_node(db, node);
		return 0;
	}

	if (i >= 0) {
		//printf("Element in node, but its not a leaf\n");
		//fflush(stdout);

		struct Node *left_child;
		struct  Node *right_child;
		for (int j = i; j <= i + 1; ++j)
		{
			struct Node *child = db->open_node(db, node->children[j]);
			if (child->n >= db->t)
			{
				//if (j == i) printf("left_child\n");
				//else printf("right_child\n");
				int child_index = j == i ? child->n-1 : 0;

				struct DBT *previous;
				if (child_index == 0)
					previous = find_next_key(db, child, key);
				else
					previous = find_previous_key(db, child, key);

				ddelete(db, child, &previous[0]);
				child->write_node(db, child);

				node->keys[i] = previous[0];
				node->values[i] = previous[1];
				node->write_node(db, node);

				return 0;
			}
			else {
				//printf("Not enough element\n");
				//fflush(stdout);
			}

			if (j == i) left_child = child;
			else right_child = child;
		}

		//unite two child : left and right
		//printf("Unite child\n");
		//fflush(stdout);

		int left_child_size = left_child->n;
		left_child->n += right_child->n+1;
		make_realloc(left_child);

		left_child->keys[left_child_size] = deep_copy(node->keys[i]); 
		left_child->values[left_child_size] = deep_copy(node->values[i]);

		int j;
		for (j = 0; j < right_child->n; j++) {
			left_child->keys[j+left_child_size+1] = right_child->keys[j];
			left_child->values[j+left_child_size+1] = right_child->values[j];
		}
		if (!left_child->leaf && !right_child->leaf) {
			for (j = 0; j < right_child->n+1; j++) {
				left_child->children[j+left_child_size+1] = right_child->children[j];
			}
		}

		delete_key(node, i);

		left_child->write_node(db, left_child);
		node->write_node(db, node);

		return ddelete(db, left_child, key);
	}

	//printf("Element not here, try to find new node for processing\n");
	//fflush(stdout);

	i = 0;
	while(i < node->n && new_memcmp(node->keys[i], *key) < 0) {
		i++;
	}
	struct Node *new_node = db->open_node(db, node->children[i]);

	if (new_node->n < db->t) {
		//printf("Not enough element in new node\n");
		//fflush(stdout);

		if (i-1 >= 0) {
			//printf("Try left child\n");
			//fflush(stdout);

			struct Node *neighbour1 = db->open_node(db, node->children[i-1]);

			if (neighbour1->n >= db->t) {
				//printf("we are in left child\n");
				//fflush(stdout);

				new_node->n++;
				make_realloc(new_node);

				struct DBT last_key = deep_copy(neighbour1->keys[neighbour1->n-1]);
				struct DBT last_value = deep_copy(neighbour1->values[neighbour1->n-1]);

				int last_child = -1;
				if (!neighbour1->leaf)
					last_child = neighbour1->children[neighbour1->n];

				struct DBT key_separator = node->keys[i-1];
				struct DBT value_separator = node->values[i-1];

				node->keys[i-1] = last_key;
				node->values[i-1] = last_value;

				int j;
				int fflg = -1;
				for (j = 0; j < new_node->n-1 && fflg == -1; j++) {
					if (new_memcmp(new_node->keys[j], last_key) >= 0) {
						fflg = j;
					}
				}

				j = new_node->n-1;
				while(j > fflg) {
					new_node->keys[j] = new_node->keys[j-1];
					new_node->values[j] = new_node->values[j-1];
					j--;
				}
				new_node->keys[fflg] = key_separator;
				new_node->values[fflg] = value_separator;

				if (!new_node->leaf) {
					for (j = new_node->n; j > fflg; j--)
						new_node->children[j] = new_node->children[j-1];
					new_node->children[fflg] = last_child;
				}

				delete_key(neighbour1, neighbour1->n-1);

				node->write_node(db, node);
				new_node->write_node(db, new_node);
				neighbour1->write_node(db, neighbour1);

				return ddelete(db, new_node, key);
			}
		}
		if (i+1 < node->n+1) {
			//printf("Try right child\n");
			//fflush(stdout);

			struct Node *neighbour2 = db->open_node(db, node->children[i+1]);
			
			if (neighbour2->n >= db->t) {
				//printf("we are in neighbour2\n");
				//fflush(stdout);

				new_node->n++;
				make_realloc(new_node);

				struct DBT first_key = deep_copy(neighbour2->keys[0]);
				struct DBT first_value = deep_copy(neighbour2->values[0]);
				int first_child = -1;
				if (!neighbour2->leaf) 
					first_child = neighbour2->children[0];

				struct DBT key_separator = node->keys[i];
				struct DBT value_separator = node->values[i];

				node->keys[i] = first_key;
				node->values[i] = first_value;

				new_node->keys[new_node->n-1] = key_separator;
				new_node->values[new_node->n-1] = value_separator;
				if (!new_node->leaf && first_child != -1)
					new_node->children[new_node->n] = first_child;

				free(neighbour2->keys[0].data);
				free(neighbour2->values[0].data);
				int k = 0;
				while (k < neighbour2->n-1) {
					neighbour2->keys[k] = neighbour2->keys[k+1];
					neighbour2->values[k] = neighbour2->values[k+1];
					k++;
				}

				if (!neighbour2->leaf) {
					k = 0;
					while (k < neighbour2->n) {
						neighbour2->children[k] = neighbour2->children[k+1];
						k++;
					}
				}
				neighbour2->n--;
				make_realloc(neighbour2);
				
				node->write_node(db, node);
				new_node->write_node(db, new_node);
				neighbour2->write_node(db, neighbour2);

				return ddelete(db, new_node, key);
			}
			else {
				//printf("Unite element neighbour2 and new_node\n");
				//fflush(stdout);

				int offset = new_node->n;
				new_node->n += neighbour2->n+1;
				make_realloc(new_node);

				new_node->keys[offset] = deep_copy(node->keys[i]);
				new_node->values[offset] = deep_copy(node->values[i]);

				int j = 0;
				while(j < neighbour2->n) {
					new_node->keys[j+offset+1] = neighbour2->keys[j];
					new_node->values[j+offset+1] = neighbour2->values[j];
					j++;
				}
				if (!new_node->leaf) {
					for (j = 0; j < neighbour2->n+1; j++)
						new_node->children[j+1+offset] = neighbour2->children[j];
				}

				delete_key(node, i);

				node->write_node(db, node);
				new_node->write_node(db, new_node);

				return ddelete(db, new_node, key);
			}
		}
		else {
			//printf("Can not open neighbour\n");
			//fflush(stdout);

			//printf("Unite element neighbour1 and new_node\n");
			struct Node *neighbour1 = NULL;

			if (i-1 >= 0) {
				neighbour1 = db->open_node(db, node->children[i-1]);
				int offset = neighbour1->n;
				neighbour1->n += new_node->n+1;
				make_realloc(neighbour1);

				neighbour1->keys[offset] = deep_copy(node->keys[i-1]);
				neighbour1->values[offset] = deep_copy(node->values[i-1]);

				int j = 0;
				while(j < new_node->n) {
					neighbour1->keys[j+offset+1] = new_node->keys[j];
					neighbour1->values[j+offset+1] = new_node->values[j];
					j++;
				}
				if (!neighbour1->leaf) {
					for (j = 0; j < new_node->n+1; j++)
						neighbour1->children[j+1+offset] = new_node->children[j];
				}

				delete_key(node, i-1);

				node->write_node(db, node);
				neighbour1->write_node(db, neighbour1);

				return ddelete(db, neighbour1, key);
			}
			else {
				printf("SUPER UGLY\n");
				fflush(stdout);
			}
		}
	}

	return ddelete(db, new_node, key);
}
int cclose(struct DB *db)
{
	//printf("Start db_close\n");
	//fflush(stdout);
	//print_status(db, db->node);
	lseek(db->block_api->fd, 0, SEEK_SET);
	write(db->block_api->fd, &db->node->num_vertix, sizeof(db->node->num_vertix));
	db->block_api->write_bitmask(db->block_api);
	db->node->write_node(db, db->node);
	db->node->close_node(db, db->node);
	db->block_api->free(db->block_api);
	free(db->block_api);
	free(db->node);
	free(db);
	//printf("End db_close\n");
	//fflush(stdout);
	return 0;
}

struct DB *dbcreate(const char *file, struct DBC *conf)
{
	struct DB *db = (struct DB *)malloc(sizeof(struct DB)*1);
	db->insert = &insert;
	db->select = &sselect;
	db->delete = &ddelete;
	db->close = &cclose;
	db->create_node = &create_node;
	db->open_node = &open_node;
	db->create_cache = &create_cache;
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
		db->node->write_node(db, db->node);
	} 
	else {
		//printf("File already created, we are reading from it\n");
		int num_vertix;
		read(db->block_api->fd, &num_vertix, sizeof(num_vertix));
		db->block_api->read_bitmask(db->block_api);
		db->node = db->open_node(db, num_vertix);
	}
	db->block_cache = db->create_cache();
	//printf("end\n");
	return db;
}