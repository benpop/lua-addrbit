SHELL = /bin/sh
UNAME = $(shell uname -s | tr '[A-Z]' '[a-z]')

ifeq ($(UNAME), darwin)
# Mac OS X
CC ?= clang
DLLFLAGS ?= -bundle -undefined dynamic_lookup
else
# assume Linux
CC ?= gcc
DLLFLAGS ?= -shared
endif

RM ?= rm -f

LUADIR ?= /usr/include

COPT ?= -O2
CVERSION = -std=c99
CWARN ?= -Wall -Wextra
CFLAGS = $(COPT) $(CVERSION) $(CWARN) -I$(LUADIR) -fPIC $(MYFLAGS)

OBJ = addrbit.o
TARG = addrbit.so

all: $(TARG)

clean:
	$(RM) $(OBJ) $(TARG)

%.so: %.o $(MYOBJ)
	$(CC) $(DLLFLAGS) -o $@ $^

%.o: %.c $(MYSRC)
	$(CC) $(CFLAGS) -c -o $@ $^
