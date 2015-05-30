all:
	gcc -Wall mydb.c block_api.c db_api.c third_party.c -std=c99 -shared -fPIC -o libdbsophia.so
sophia:
	make -C sophia/