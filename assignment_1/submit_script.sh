#!/bin/bash
#PBS -l nodes=2:ppn=20,walltime=1:00
#PBS -N MPIsample
#PBS -q edu_shared
#PBS -m abe
#PBS -M rkaran3@uic.edu
#PBS -e mpitest.err
#PBS -o mpitest.out
#PBS -d /export/home/rkaran3/working_directory/test

module load tools/mpich2-1.5-gcc
mpirun -machinefile $PBS_NODEFILE -np $PBS_NP ./__main.o