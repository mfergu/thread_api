
CC=clang
CFLAGS=-Wall -g -c

BINS=libmythreads.a main 

all: $(BINS)

%: %.c                                                                                                        
	$(CC) $(CFLAGS) -o $@ $? -pthread 

libmythreads.a: mythreads.o list.o queue.o lock.o
	ar -cvr libmythreads.a mythreads.o list.o queue.o lock.o

clean: 
	rm -rf $(BINS) *.o *.dSYM
