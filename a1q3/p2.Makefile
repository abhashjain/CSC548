#Author : 
#1. ajain28 Abhash Jain
#2. asrivas3 Abhishek Kumar Srivastava
#CSC548 - Assignment-1 - Question 3
all: p2_mpi.c
	mpicc -O3 -o p2_mpi p2_mpi.c p2_func.c -lm

clean:
	rm -f *.o p2_mpi
