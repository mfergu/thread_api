
CC=clang
#CFLAGS=-Wall -g -DNDEBUG -O3
CFLAGS=-Wall -g -O3

BINS=libmythreads.a 

libmythreads.a: mythreads.o list.o queue.o lock.o
	ar -cvr libmythreads.a mythreads.o list.o queue.o lock.o

clean: 
	rm -rf $(BINS) *.o *.dSYM
