TOP = .
CC ?= gcc

CORE_DIR      = $(TOP)/concord/core
INCLUDE_DIR   = $(TOP)/concord/include
GENCODECS_DIR = $(TOP)/concord/gencodecs

SRC_DIR		  = $(TOP)/src
OUT_DIR 	  = $(TOP)/bin

CFLAGS += -pthread \
           -I$(INCLUDE_DIR) -I$(CORE_DIR) -I$(GENCODECS_DIR)


CFLAGS += -std=c99 
CFLAGS += -Wpedantic
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Waggregate-return
CFLAGS += -Wbad-function-cast
CFLAGS += -Wcast-align
CFLAGS += -Wcast-qual
CFLAGS += -Wfloat-equal
CFLAGS += -Wformat=2
CFLAGS += -Wlogical-op
CFLAGS += -Wmissing-include-dirs
CFLAGS += -Wnested-externs
CFLAGS += -Wpointer-arith
CFLAGS += -Wredundant-decls
CFLAGS += -Wsequence-point
CFLAGS += -Wshadow
CFLAGS += -Wswitch
CFLAGS += -Wundef
CFLAGS += -Wunreachable-code
CFLAGS += -Wwrite-strings
CFLAGS += -Wno-discarded-qualifiers

LDFLAGS  = -L$(TOP)/concord/lib
LDLIBS   = -ldiscord -lcurl

CFLAGS +=  $(LDFLAGS)

all: outdir release debug asan

release: $(CORE_DIR)
	$(CC) -O2 $(CFLAGS) \
	$(SRC_DIR)/main.c $(LDLIBS) -o $(OUT_DIR)/$@ 

debug: $(CORE_DIR)
	$(CC) -O0 -DDEBUG -g $(CFLAGS) \
	$(SRC_DIR)/main.c $(LDLIBS) -o $(OUT_DIR)/$@ 

asan: $(CORE_DIR)
	$(CC) -O0 -DDEBUG -fsanitize=address -fsanitize=undefined -fsanitize=leak \
	-g $(CFLAGS) $(SRC_DIR)/main.c $(LDLIBS) -o $(OUT_DIR)/$@ 


$(CORE_DIR):
	git submodule update --init --recursive
	cd concord && git checkout update/gencodecs \
			   && make \
			   && PREFIX=.. make install 

outdir:
	mkdir -p $(OUT_DIR)

clean:
	rm -f $(OUT_DIR)/*

.PHONY: all clean outdir