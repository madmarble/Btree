all:
	@$(MAKE) -C third_party/sophia/db/ libsophia.a
	cp third_party/sophia/db/libsophia.a ./
	gcc db_sphia.c -std=c99 -shared -fPIC -o libdbsophia.so -lsophia -L. -Wall -lpthread
