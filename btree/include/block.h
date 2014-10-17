#pragma once
#include "btree.h"
struct DB * read_block(struct DBC *conf, int block_num);
void write_block(struct DB *x);