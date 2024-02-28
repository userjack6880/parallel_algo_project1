#!/bin/bash
#SBATCH --account=class-cse4163
#SBATCH --job-name=Work$(printf "%0*d" 2 $1)P
#SBATCH --nodes=1
#SBATCH --ntasks=$1
#SBATCH --cpus-per-task=1
#SBATCH --time=00:20:00
#SBATCH --partition=ptolemy
#SBATCH --no-reque
#SBATCH --qos=class-cse4163
module load openmpi

mpirun -np $1 ./project1 hard_sample.dat sol_hard.$(printf "%0*d" 2 $1) $3 $4 $5 >& $2.$(printf "%0*d" 2 $1)p
