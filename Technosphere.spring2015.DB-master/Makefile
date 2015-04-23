all:
	gcc -Wall mydb.c block_api.c main.c -o main
	gcc -Wall mydb.c block_api.c -std=c99 -shared -fPIC -o libdbsophia.so
sophia:
	make -C sophia/
