#include "mydb.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	printf("Hello user\n");
	struct DBC conf;
	conf.db_size = 1024*1024;
	conf.page_size = 4096;
	struct DB *db = dbcreate(argv[1], &conf);
	print_status(db, db->node);

	printf("Please choose what to do\n");
	printf("1 - insert element\n");
	printf("2 - search element\n");
	printf("3 - print database\n");

	int choose;
	scanf("%d", &choose);
	printf("\n");
	if (choose == 1) {
		struct DBT key, value;
		key.size = 1;
		key.data = malloc(key.size);
		value.size = 1;
		value.data = malloc(value.size);
		printf("Please, type data for key and value\n");
		scanf("%d %d", (char *)key.data, (char *)value.data);
		
		db_insert(db, key.data, key.size, value.data, value.size);

		free(key.data);
		free(value.data);

		//print_status(db);
		db_close(db);
	}
	if (choose == 2) {
		struct DBT key, value;
		key.size = 1;
		key.data = malloc(key.size);
		value.size = 1;
		printf("Please, type data for key\n");
		scanf("%d", (char *)key.data);

		if (db_select(db, key.data, key.size, &value.data, &value.size) == -1)
			printf("Cant find key in db\n");
		else {
			printf("This is data for value : %d\n", *(char *)value.data);
			printf("This is size for value : %d\n", value.size);
		}
		fflush(stdout);

		free(key.data);
		//free(value.data);

		//print_status(db);
		db_close(db);
	}
	if (choose == 3) {
		db->block_api->bitmap->show(db->block_api->bitmap);
		print_status(db, db->node);
		db_close(db);
	}
	
	return 0;
}