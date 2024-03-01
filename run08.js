#!/bin/bash
#SBATCH --account=class-cse4163
#SBATCH --nodes=1
#SBATCH --ntasks=8
#SBATCH --cpus-per-task=1
#SBATCH --time=00:20:00
#SBATCH --partition=ptolemy
#SBATCH --no-reque
#SBATCH --qos=class-cse4163
module load openmpi
mpirun -np 8 ./project1 hard_sample.dat $4/sol_hard_$3.08 $1 $2 >& $4/results_$3.08p
