#!/bin/bash
# starting packet 1
# no increase
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 1 0 0 p1_no_inc_no_dec
done
# increase, no decrease
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 1 1 0 p1_no_inc_no_dec
done
# increase, decrease
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 1 1 1 p1_no_inc_no_dec
done

# starting packet 10
# no increase
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 10 0 0 p1_no_inc_no_dec
done
# increase, no decrease
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 10 1 0 p1_no_inc_no_dec
done
# increase, decrease
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 10 1 1 p1_no_inc_no_dec
done