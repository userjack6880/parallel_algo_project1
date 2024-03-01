#!/bin/bash
#SBATCH --account=class-cse4163
#SBATCH --nodes=1
#SBATCH --ntasks=2
#SBATCH --cpus-per-task=1
#SBATCH --time=00:20:00
#SBATCH --partition=ptolemy
#SBATCH --no-reque
#SBATCH --qos=class-cse4163
module load openmpi
mpirun -np 2 ./project1 hard_sample.dat $4/sol_hard_$3.02 $1 $2 >& $4/results_$3.02p
