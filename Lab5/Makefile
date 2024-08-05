CFLAGS := -g -Wall -Werror
LOADLIBES := -lm
LDFLAGS := -pthread
#TARGETS := server client
TARGETS := server client

# Make sure that 'all' is the first target
all: depend $(TARGETS)

server: server.o common.o
client: client.o common.o

clean:
	rm -rf core *.o $(TARGETS)

realclean: clean
	rm -rf *~ *.bak .depend *.log *.out

tags:
	etags *.c *.h

depend:
	$(CC) -MM *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif