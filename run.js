#!/bin/bash

# optional arguments
numTasks=1
minPacket=1
incPacket=1
decPacket=1
resultName="results"

if [ "$#" -ge 1 ]; then
  numTasks="$1"
fi
if [ "$#" -ge 2 ]; then
  resultName="$2"
fi
if [ "$#" -ge 3 ]; then
  minPacket="$3"
fi
if [ "$#" -ge 4 ]; then
  incPacket="$4"
fi
if [ "$#" -ge 5 ]; then
  decPacket="$5"
fi

#SBATCH --account=class-cse4163
#SBATCH --job-name=Work$(printf "%0*d" 2 $numTasks)P
#SBATCH --nodes=1
#SBATCH --ntasks=$numTasks
#SBATCH --cpus-per-task=1
#SBATCH --time=00:20:00
#SBATCH --partition=ptolemy
#SBATCH --no-reque
#SBATCH --qos=class-cse4163
module load openmpi

mpirun -np $numTasks ./project1 hard_sample.dat sol_hard.$(printf "%0*d" 2 $numTasks) $minPacket $incPacket $decPacket >& $resultName.$(printf "%0*d" 2 $numTasks)p
