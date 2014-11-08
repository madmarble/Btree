#pragma once
struct DBT {
	void *data;
	size_t size;
};
struct DBC {
	size_t db_size;
	size_t chunk_size;
	/* For future uses - maximum cached memory size
	* 16MB by default
	* size_t mem_size; */
};
struct Node{
	char leaf;
	int n;
	int own_tag;
	int parent;
	int *neighbours;
	struct DBT *keys;
	struct DBT *values;
	struct DBC *conf;
};
struct Disk {
	char *exist_or_not;
	int count_blocks;
	const char *file;
	int first_empty;
	int start_offset;
	int root_offset;
	int db_size;
	int chunk_size;
	struct Node * (*read_block)(struct Disk *x, int block_num);
	void (*write_block)(struct Disk *x, struct Node *res);
};
struct Node * read_block(struct Disk *x, int block_num);
void write_block(struct Disk *x, struct Node *res);
void initialize_fields(struct Disk *res);
void write_disk(struct Disk *disk);
struct Disk * create_disk(const char *file);
struct Disk * read_disk(const char *file, struct DBC *conf);