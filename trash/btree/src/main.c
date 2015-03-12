#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btree.h"
#include "block.h"
int
main(int argc, char **argv)
{
	printf("Hello! We start programm\n");
	char *s = (char *)malloc(1000*sizeof(*s));
	int *t = (int *)calloc(1, sizeof(*t));
	*t = 4;
	int *val_t = (int *)calloc(1, sizeof(*val_t));
	*val_t = 10;
	while(scanf("%s", s) > 0) {
		if (!strcmp(s, "put")) {
			struct DB *x = dbopen("example.txt", NULL);
			printf("You choose put element\n");
			db_put(x, t, sizeof(*t), val_t, sizeof(*val_t));
			printf("We end put element in the tree\n");
			db_close(x);
		}
		if (!strcmp(s, "search")) {
			struct DB *x = dbopen("example.txt", NULL);
			printf("You choose search element\n");
			void *tmp = (void *)val_t;
			size_t tmp_size = sizeof(val_t);
			db_get(x, t, sizeof(*t), &tmp, &tmp_size);
			if (tmp == NULL) printf("We cant find element\n");
			else printf("value for key is %d\n", *(int *)tmp);
			printf("We end search element in the tree\n");
			db_close(x);
		}
		if (!strcmp(s, "delete")) {
			struct DB *x = dbopen("example.txt", NULL);
			printf("You choose delete element\n");
			db_del(x, t, sizeof(*t));
			printf("We end deleting element in the tree\n");
			db_close(x);
		}
		printf("Start printing tree\n");
		struct DB *x = dbopen("example.txt", NULL);
		int i;
		printf("Size is %d\n", x->node->n);
		for(i = 0; i < x->node->n; i++) {
			printf("Size of key is %d and size of value is %d\n", (char)x->node->keys[i].size, (char)x->node->values[i].size);
			printf("Key is %d and value is %d\n", *(char *)x->node->keys[i].data, *(char *)x->node->values[i].data);
		}
		db_close(x);
		printf("End printing tree\n");
		(*t)++;(*val_t)++;
	}
	free(t);
	free(val_t);
	free(s);
	return 0;
}