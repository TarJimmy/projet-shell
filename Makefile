.PHONY: all, clean

# Disable implicit rules
.SUFFIXES:

CC=gcc
CFLAGS=-Wall -g
SRCDIR=src
INCLUDEDIR=include
# Note: -lnsl does not seem to work on Mac OS but will
# probably be necessary on Solaris for linking network-related functions 
#LIBS += -lsocket -lnsl -lrt
LIBS+=-lpthread

SRCS = $(wildcard $(SRCDIR)/*.c)
INCLUDE = $(SRCS:$(SRCDIR)/%.c=%.h)
OBJS = $(SRCS:$(SRCDIR)/%.c=%.o)
INCLDIR = -I$(INCLUDEDIR)

ifdef DEBUG
CFLAGS += -DDEBUG=0
endif

all: shell

%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLDIR) -c -o $@ $<

%.c: %.o $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)

shell: $(OBJS)
	$(CC) $(OBJS) -o $@ $(LIBS)
	rm -f */*.o

clean:
	rm -f shell *.o *.txt