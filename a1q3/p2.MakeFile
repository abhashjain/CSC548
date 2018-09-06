all: p2_mpi.c
	mpicc -O3 -o p2_mpi p2_mpi.c p2_func.c -lm

clean:
	rm -f *.o p2_mpi
