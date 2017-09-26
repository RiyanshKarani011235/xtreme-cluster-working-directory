#!/bin/bash

#PBS -l mem=20gb
#PBS -l walltime=00:05:00
#PBS -l nodes=1:ppn=8
#PBS -j oe
#PBS –m abe
#PBS -M rkaran3@uic.edu
#PBS -N hello_world
#PBS -d /export/home/rkaran3/working_directory/test

mpirun -machinefile $PBS_NODEFILE -np $PBS_NP ./outp./outp./output
