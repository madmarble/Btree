CC=gcc
CFLAGS=-O2 -Wall -Werror -Wformat-security -Wignored-qualifiers -Winit-self -Wswitch-default -Wfloat-equal -Wshadow -Wpointer-arith -Wtype-limits -Wempty-body -Wlogical-op -Wstrict-prototypes -Wold-style-declaration -Wold-style-definition -Wmissing-parameter-type -Wmissing-field-initializers -Wnested-externs -Wno-pointer-sign -std=gnu99

SOURCES=btree.c
HEADERS=btree.h conf.h block.h
OBJECTS=$(SOURCES:.c=.o)

all: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -o stat
