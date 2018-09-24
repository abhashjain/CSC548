#Author :ajain28 Abhash Jain
CC = gcc
CFLAGS = -I -g -lm -Wincompatible-pointer-types
INCLUDE_PATH = .

default:	mpi

mpi:		my_mpi.o p1.o
			$(CC) $(CFLAGS) -o p1_mpi my_mpi.o p1.o

my_mpi.o:	my_mpi.c $(INCLUDE_PATH)/my_mpi.h
				$(CC) $(CFLAGS) -c my_mpi.c

p1.o:	p1.c
			$(CC) $(CFLAGS) -c p1.c

clean:
	rm -f *.o p1_mpi
