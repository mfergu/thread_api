
CC=clang-3.6
CFLAGS=-Wall -g -c

BINS=libmythreads.a

all: $(BINS)

%: %.c                                                                                                        
	$(CC) $(CFLAGS) -o $@ $? -pthread 

libmythreads.a: mythreads.o list.o
	ar -cvr libmythreads.a mythreads.o list.o

clean: 
	rm -rf $(BINS) *.o *.dSYM
