#!/bin/bash
#SBATCH --account=class-cse4163
#SBATCH --job-name=Work04P
#SBATCH --nodes=1
#SBATCH --ntasks=4
#SBATCH --cpus-per-task=1
#SBATCH --time=00:20:00
#SBATCH --partition=ptolemy
#SBATCH --no-reque
#SBATCH --qos=class-cse4163
module load openmpi
mpirun -np 4 ./project1 hard_sample.dat sol_hard_$4.04 $1 $2 $3 >& results_$4.04p
