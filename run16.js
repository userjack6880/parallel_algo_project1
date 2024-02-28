#!/bin/bash
#SBATCH --account=class-cse4163
#SBATCH --job-name=Work16P
#SBATCH --nodes=1
#SBATCH --ntasks=16
#SBATCH --cpus-per-task=1
#SBATCH --time=00:20:00
#SBATCH --partition=ptolemy
#SBATCH --no-reque
#SBATCH --qos=class-cse4163
module load openmpi
mpirun -np 16 ./project1 hard_sample.dat sol_hard_$4.16 $1 $2 $3 >& results_$4.16p
