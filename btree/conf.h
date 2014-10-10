#include "btree.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

void dbcreate(const char *file, struct DBC *conf)
{
	conf->fd = open(file, O_CREAT | O_RDWR);
	conf->db_size = 512 * 1024 * 1024;
	conf->chunk_size = 4 * 1024;
	conf->start_offset += 2 * sizeof(size_t) + sizeof(int);
	conf->t = conf->chunk_size / 2 - 1;
	write(conf->fd, &conf->db_size, sizeof(size_t));
	write(conf->fd, &conf->chunk_size, sizeof(size_t));
	conf->start_offset += conf->db_size / conf->chunk_size + conf->db_size % conf->chunk_size;
	write(conf->fd, &conf->t, sizeof(int));
	conf->start_offset = conf->start_offset / conf->chunk_size + (conf->start_offset % conf->chunk_size ? 1 : 0);

	//reading byte_massive
	//todo with bit!
	conf->count_blocks = conf->db_size / conf->chunk_size;
	conf->exist_or_not = (char *)calloc(conf->count_blocks, sizeof(char));
	int i;
	for (i = 0; i < conf->db_size / conf->chunk_size; i++) {
		write(conf->fd, &conf->exist_or_not[i], sizeof(char));
	}
	conf->root_offset = conf->start_offset;
	conf->first_empty = -1;
	printf("Settings were created\n");
	return;
}

int dbopen(const char *file, struct DBC *conf)
{
	conf->fd = open(file, O_RDWR);
	if (conf->fd == -1) {
		fprintf(stderr, "Cant open file %s!\n", file);
		return 1;
	}
	conf->start_offset += 2 * sizeof(size_t) + sizeof(int);
	read(conf->fd, &conf->db_size, sizeof(size_t));
	read(conf->fd, &conf->chunk_size, sizeof(size_t));
	conf->start_offset += conf->db_size / conf->chunk_size + conf->db_size % conf->chunk_size;
	//redo
	read(conf->fd, &conf->t, sizeof(int));
	conf->start_offset = conf->start_offset / conf->chunk_size + (conf->start_offset % conf->chunk_size ? 1 : 0);

	//reading byte_massive
	//todo with bit!
	conf->count_blocks = conf->db_size / conf->chunk_size;
	conf->exist_or_not = (char *)malloc(sizeof(char) * (conf->count_blocks));
	int i;
	for (i = 0; i < conf->db_size / conf->chunk_size; i++) {
		read(conf->fd, &conf->exist_or_not[i], sizeof(char));
	}
	conf->root_offset = conf->start_offset;
	conf->first_empty = -1;
	printf("Settings were created\n");
	return 0;
}