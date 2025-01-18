# This Makefile assumes the top folder has been built
TOP = .
CC ?= cc

CORE_DIR      = $(TOP)/concord/core
INCLUDE_DIR   = $(TOP)/concord/include
GENCODECS_DIR = $(TOP)/concord/gencodecs

STD_BOTS   = main
VOICE_BOTS = 

BOTS += $(STD_BOTS)

CFLAGS  += -O0 -g -pthread \
           -I$(INCLUDE_DIR) -I$(CORE_DIR) -I$(GENCODECS_DIR) die.c die.h structs.h

CFLAGS += -std=c99 
CFLAGS += -Wpedantic
CFLAGS += -Wall
# CFLAGS += -fsanitize=address
# CFLAGS += -fsanitize=undefined
# CFLAGS += -fsanitize=leak
# CFLAGS += -Wwrite-strings

LDFLAGS  = -L$(TOP)/concord/lib
LDLIBS   = -ldiscord -lcurl

all: $(BOTS)

voice:
	@ CFLAGS=-DCCORD_VOICE BOTS=$(VOICE_BOTS) $(MAKE)

echo:
	@ echo -e 'CC: $(CC)\n'
	@ echo -e 'STD_BOTS: $(STD_BOTS)\n'
	@ echo -e 'VOICE_BOTS: $(VOICE_BOTS)\n'

clean:
	@ rm -f $(STD_BOTS) $(VOICE_BOTS)

.PHONY: all echo clean
