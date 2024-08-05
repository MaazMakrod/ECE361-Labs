CFLAGS := -g -Wall -Werror
LOADLIBES := -lm
#TARGETS := server deliver
TARGETS := server deliver

# Make sure that 'all' is the first target
all: depend $(TARGETS)

server: server.o common.o
deliver: deliver.o common.o

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