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
	//print_node(x);
	//print_node(y);
	//print_node(z);

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
	//print_node(z);

	y->n = db->t-1;
	//print_node(y);

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
	//print_node(x);

	z->write_node(db, z);
	z->close_node(db, z);

	y->write_node(db, y);
	y->close_node(db, y);

	x->write_node(db, x);
	return 0;
}

int new_memcmp(void *f1, void *f2, size_t sf1, size_t sf2)
{
	int res;
	if (sf1 > sf2)
		res = 1;
	else if (sf2 > sf1)
		res = -1;
	else
		res = memcmp(f1, f2, sf1);
	return res;
}

int insert_nonfull(struct DB *db, struct DBT *key, struct DBT *value)
{
	int i = db->node->n-1;
	//print_node(db->node);
	if (db->node->leaf) {
		if (i > 0) {
			while(i >= 0 &&  new_memcmp(db->node->keys[i].data, key->data, db->node->keys[i].size, key->size) > 0) {
				i--;
			}
			if (new_memcmp(db->node->keys[i].data, key->data, db->node->keys[i].size, key->size) == 0) {
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
			//printf("We have %d elements\n", db->node->n);
			//fflush(stdout);
			db->node->keys = (struct DBT *)realloc(db->node->keys, (db->node->n+1)*sizeof(*db->node->keys));
			db->node->values = (struct DBT *)realloc(db->node->values, (db->node->n+1)*sizeof(*db->node->values));
		}
		else {
			//printf("We dont have any element yet\n");
			//fflush(stdout);
			db->node->keys = (struct DBT *)malloc((db->node->n+1)*sizeof(*db->node->keys));
			db->node->values = (struct DBT *)malloc((db->node->n+1)*sizeof(*db->node->values));
		}

		while(i >= 0 &&  new_memcmp(db->node->keys[i].data, key->data, db->node->keys[i].size, key->size) > 0) {
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
		//printf("azaza\n");
		//fflush(stdout);
		db->node->write_node(db, db->node);
		//printf("azaza\n");
		//fflush(stdout);
	} else {
		while(i >= 0 && new_memcmp(db->node->keys[i].data, key->data, db->node->keys[i].size, key->size) > 0) {
			i--;
		}
		if (i >= 0 && new_memcmp(db->node->keys[i].data, key->data, db->node->keys[i].size, key->size) == 0) {
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
			if (new_memcmp(db->node->keys[i].data, key->data, db->node->keys[i].size, key->size) < 0)
				i++;
			else if (new_memcmp(db->node->keys[i].data, key->data, db->node->keys[i].size, key->size) == 0) {
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
		insert_nonfull(db, key, value);
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
	int i = 0;
	int flg = 1;
	struct Node *node = db->node;
	int first = 0;
	while(flg) {
		while(i < node->n && new_memcmp(node->keys[i].data, key->data, node->keys[i].size, key->size) < 0) {
			i++;
		}
		if (i < node->n &&  new_memcmp(node->keys[i].data, key->data, node->keys[i].size, key->size) == 0) {
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
	printf("In delete_key\n");
	fflush(stdout);
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
	print_node(node);
	return 0;
}

int find_key(struct Node *node, struct DBT *key)
{
	int i = node->n-1;
	while(i >= 0 && new_memcmp(node->keys[i].data, key->data, node->keys[i].size, key->size)) {
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

struct DBT * resursive_delete_previous(struct DB *db, struct Node *node, int index)
{
	printf("In resursive_delete_previous\n");
	fflush(stdout);

	printf("Node\n");
	print_node(node);

	if (node->leaf) {
		printf("maybe dangerous, checking this\n");
		delete_key(node, index);
		node->write_node(db, node);
		print_node(node);
		return NULL;
	}

	struct Node *left_child = db->open_node(db, node->children[index]);

	printf("left_child\n");
	fflush(stdout);
	print_node(left_child);

	if (left_child->n >= db->t) {
		// we will delete last element in left child and return key and value
		printf("we in left_child\n");
		fflush(stdout);

		int i = left_child->n-1;
		if (left_child->leaf) {
			struct DBT return_key = deep_copy(left_child->keys[i]);
			struct DBT return_value = deep_copy(left_child->values[i]);

			delete_key(left_child, i);
			left_child->write_node(db, left_child);

			struct DBT *return_pair = (struct DBT *)malloc(2*sizeof(*return_pair));
			return_pair[0] = return_key;
			return_pair[1] = return_value;
			return return_pair;
		}
		else {
			return resursive_delete_previous(db, left_child, i);
		}
	}

	struct Node *right_child = db->open_node(db, node->children[index+1]);

	printf("right_child\n");
	fflush(stdout);
	print_node(right_child);

	if (right_child->n >= db->t) {
		//we will delete first element in right child and return key and value
		printf("we in right_child\n");
		fflush(stdout);

		int i = 0;
		if (right_child->leaf) {
			struct DBT return_key = deep_copy(right_child->keys[i]);
			struct DBT return_value = deep_copy(right_child->values[i]);

			delete_key(right_child, i);
			right_child->write_node(db, right_child);

			struct DBT *return_pair = (struct DBT *)malloc(2*sizeof(*return_pair));
			return_pair[0] = return_key;
			return_pair[1] = return_value;
			return return_pair;
		}
		else {
			return resursive_delete_previous(db, right_child, i);
		}
	}

	//unite two child : left and right
	printf("Unite child\n");
	fflush(stdout);

	int left_child_size = left_child->n;
	left_child->n += right_child->n+1;
	make_realloc(left_child);

	left_child->keys[left_child_size] = deep_copy(node->keys[index]); ///??
	left_child->values[left_child_size] = deep_copy(node->values[index]);

	int i;
	for (i = 0; i < right_child->n; i++) {
		left_child->keys[i+left_child_size+1] = right_child->keys[i];
		left_child->values[i+left_child_size+1] = right_child->values[i];
	}
	if (!left_child->leaf && !right_child->leaf) {
		for (i = 0; i < right_child->n+1; i++) {
			left_child->children[i+left_child_size+1] = right_child->children[i];
		}
	}

	delete_key(node, index);
	left_child->write_node(db, left_child);
	node->write_node(db, node);

	return resursive_delete_previous(db, left_child, left_child_size);
}

int ddelete(struct DB *db, struct Node *node, struct DBT *key)
{
	printf("This is our key : %d %s\n", key->size, (char *)key->data);
	fflush(stdout);

	print_node(node);
	fflush(stdout);

	int i = find_key(node, key);
	if (i >= 0 && node->leaf) {
		printf("Leaf and we find element\n");
		fflush(stdout);
		delete_key(node, i);
		node->write_node(db, node);
		return 0;
	}

	if (i < 0 && node->leaf) {
		printf("Can not find elem in btree\n");
		fflush(stdout);
		return 0;
	}

	if (i >= 0 && !node->leaf) {
		printf("Element in node, but its not a leaf\n");
		fflush(stdout);

		struct DBT *previous_pair = resursive_delete_previous(db, node, i);
		if (previous_pair == NULL) {
			return 0;
		}
		else {
			node->keys[i] = previous_pair[0];
			node->values[i] = previous_pair[1];
			node->write_node(db, node);
			return 0;
		}
	}

	//printf("Element not here, try to find new node for processing\n");
	fflush(stdout);

	i = node->n-1;
	while(i >= 0 && new_memcmp(node->keys[i].data, key->data, node->keys[i].size, key->size) > 0) {
		i--;
	}

	//printf("We open child number %d\n", i+1);
	struct Node *new_node = db->open_node(db, node->children[i+1]);
	//printf("New node number %d and size %d\n", new_node->num_vertix, new_node->n);

	if (new_node->n < db->t) {
		//printf("Not enough element in new node\n");
		fflush(stdout);
		//printf("Print new_node\n");
		//print_node(new_node);
		fflush(stdout);

		if (i >= 0) {
			//printf("Try left child\n");
			fflush(stdout);
			struct Node *neighbour1 = db->open_node(db, node->children[i]);
			//printf("neighbour1 number %d and size %d\n", neighbour1->num_vertix, neighbour1->n);
			if (neighbour1->n >= db->t) {
				//printf("we are in left child\n");
				fflush(stdout);
				//print_node(neighbour1);
				fflush(stdout);

				new_node->n++;
				make_realloc(new_node);

				struct DBT last_key = neighbour1->keys[neighbour1->n-1];
				struct DBT last_value = neighbour1->values[neighbour1->n-1];
				int last_child = -1;
				if (!neighbour1->leaf)
					last_child = neighbour1->children[neighbour1->n];

				struct DBT key_separator = node->keys[i];
				struct DBT value_separator = node->values[i];

				node->keys[i] = last_key;
				node->values[i] = last_value;

				int j = new_node->n-1;
				while(j > 0) {
					new_node->keys[j] = new_node->keys[j-1];
					new_node->values[j] = new_node->values[j-1];
					j--;
				}
				new_node->keys[0] = key_separator;
				new_node->values[0] = value_separator;
				if (!new_node->leaf) {
					for (j = new_node->n; j > 0; j--)
						new_node->children[j] = new_node->children[j-1];
					new_node->children[0] = last_child;
				}

				neighbour1->n--;
				make_realloc(neighbour1);

				node->write_node(db, node);
				new_node->write_node(db, new_node);
				neighbour1->write_node(db, neighbour1);

				return ddelete(db, new_node, key);
			}
		}
		if (i+2 < node->n+1) {
			//printf("Try right child\n");
			fflush(stdout);

			struct Node *neighbour2 = db->open_node(db, node->children[i+2]);
			//printf("neighbour2 number %d and size %d\n", neighbour2->num_vertix, neighbour2->n);
			
			if (neighbour2->n >= db->t) {
				//printf("we are in neighbour2\n");
				fflush(stdout);
				//print_node(neighbour2);
				fflush(stdout);

				new_node->n++;
				make_realloc(new_node);

				struct DBT first_key = neighbour2->keys[0];
				struct DBT first_value = neighbour2->values[0];
				int first_child = -1;
				if (!neighbour2->leaf) 
					first_child = neighbour2->children[0];

				int j = 0;
				while (j < neighbour2->n-1) {
					neighbour2->keys[j] = neighbour2->keys[j+1];
					neighbour2->values[j] = neighbour2->values[j+1];
					j++;
				}
				if (!neighbour2->leaf)
					for (j = 0; j < neighbour2->n; j++) {
						neighbour2->children[j] = neighbour2->children[j+1];
					}
				neighbour2->n--;
				make_realloc(neighbour2);
					
				struct DBT key_separator = node->keys[i+1];
				struct DBT value_separator = node->values[i+1];

				node->keys[i+1] = first_key;
				node->values[i+1] = first_value;

				new_node->keys[new_node->n-1] = key_separator;
				new_node->values[new_node->n-1] = value_separator;
				if (!new_node->leaf && first_child != -1)
					new_node->children[new_node->n] = first_child;

				node->write_node(db, node);
				new_node->write_node(db, new_node);
				neighbour2->write_node(db, neighbour2);

				return ddelete(db, new_node, key);
			}
			else {
				//printf("Unite element neighbour2 and new_node\n");
				fflush(stdout);
				printf("neighbour2\n");
				//print_node(neighbour2);
				fflush(stdout);

				int offset = new_node->n;
				new_node->n += neighbour2->n+1;
				make_realloc(new_node);

				new_node->keys[offset] = deep_copy(node->keys[i+1]);
				new_node->values[offset] = deep_copy(node->values[i+1]);

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

				delete_key(node, i+1);

				node->write_node(db, node);
				new_node->write_node(db, new_node);

				return ddelete(db, new_node, key);
			}
		}
		else {
			//printf("Can not open neighbour\n");
			fflush(stdout);

			//printf("Unite element neighbour1 and new_node\n");
			struct Node *neighbour1 = NULL;
			fflush(stdout);

			if (i >= 0) {
				neighbour1 = db->open_node(db, node->children[i]);
				int offset = neighbour1->n;
				neighbour1->n += new_node->n+1;
				make_realloc(neighbour1);

				neighbour1->keys[offset] = deep_copy(node->keys[i]);
				neighbour1->values[offset] = deep_copy(node->values[i]);

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

				delete_key(node, i);

				node->write_node(db, node);
				neighbour1->write_node(db, neighbour1);

				return ddelete(db, neighbour1, key);
			}
			else {
				//printf("SUPER UGLY\n");
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
	//printf("end\n");
	return db;
}