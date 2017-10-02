#!/bin/bash

#PBS -l mem=20gb
#PBS -l walltime=00:05:00
#PBS -l nodes=1:ppn=8
#PBS –m abe
#PBS -M rkaran3@uic.edu
#PBS -N hello_world
#PBS -j oe
#PBS -d /export/home/rkaran3/working_directory/test

mpirun -machinefile $PBS_NODEFILE -np $PBS_NP ./output
