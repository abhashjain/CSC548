#!/bin/bash
rm -rf fn-*.dat
prun ./p2_mpi 10 0 1
prun ./p2_mpi 100 0 1
prun ./p2_mpi 1000 0 1
prun ./p2_mpi 10000 0 1
sdiff -s 10.dat fn-10.dat
sdiff -s 100.dat fn-100.dat
sdiff -s 1000.dat fn-1000.dat
sdiff -s 10000.dat fn-10000.dat
echo "Blocking Test case Passed"
rm -rf fn-*.dat
prun ./p2_mpi 10 1 1
prun ./p2_mpi 100 1 1 
prun ./p2_mpi 1000 1 1 
prun ./p2_mpi 10000 1 1 
sdiff -s 10.dat fn-10.dat
sdiff -s 100.dat fn-100.dat
sdiff -s 1000.dat fn-1000.dat
sdiff -s 10000.dat fn-10000.dat
echo "Non-Blocking Test case Passed"
rm -rf fn-*.dat
prun ./p2_mpi 10 0 0 
prun ./p2_mpi 100 0 0 
prun ./p2_mpi 1000 0 0 
prun ./p2_mpi 10000 0 0 
sdiff -s 10.dat fn-10.dat
sdiff -s 100.dat fn-100.dat
sdiff -s 1000.dat fn-1000.dat
sdiff -s 10000.dat fn-10000.dat
echo "MPI_Gather Test case Passed"

