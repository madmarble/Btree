CC=gcc
CFLAGS = -O2 -Wall -std=gnu99
CFLAGS += -Werror -Wformat-security -Wignored-qualifiers -Winit-self\
		-Wswitch-default -Wfloat-equal -Wshadow -Wpointer-arith\
		-Wtype-limits -Wempty-body -Wlogical-op -Wstrict-prototypes\
		-Wold-style-declaration -Wold-style-definition\
		-Wmissing-parameter-type -Wmissing-field-initializers\
		-Wnested-externs -Wno-pointer-sign 

SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
#BIN_DIR = $(BUILD_DIR)/bin
LIB_DIR = lib
CFLAGS += -I $(INCLUDE_DIR)

make_path = $(addsuffix $(1), $(basename $(subst $(2), $(3), $(4))))
# Takes path list with source files and returns pathes to related objects.
src_to_obj = $(call make_path,.o, $(SRC_DIR), $(OBJ_DIR), $(1))
all: $(LIB_DIR)/btree.so
$(LIB_DIR)/btree.so: $(OBJ_DIR)/btree.o $(OBJ_DIR)/block.o $(OBJ_DIR)/operations.o
	$(CC) $(CFLAGS) $(filter %.o, $^) -shared -o $@
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -fPIC -c  -o   $(call src_to_obj, $<) $< 
clean:
	rm $(OBJ_DIR)/*.o
