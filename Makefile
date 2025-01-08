# This Makefile assumes the top folder has been built
TOP = ./concord
CC ?= cc

CORE_DIR      = $(TOP)/core
INCLUDE_DIR   = $(TOP)/include
GENCODECS_DIR = $(TOP)/gencodecs

STD_BOTS   = main
VOICE_BOTS = 

BOTS += $(STD_BOTS)

CFLAGS  += -O0 -g -pthread -Wall \
           -I$(INCLUDE_DIR) -I$(CORE_DIR) -I$(GENCODECS_DIR)
LDFLAGS  = -L$(TOP)/lib
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
