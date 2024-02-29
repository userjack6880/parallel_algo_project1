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
  sbatch run$run.js 1 1 0 p1_inc_no_dec
done
# increase, decrease
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 1 1 1 p1_inc_dec
done

# starting packet 10
# no increase
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 10 0 0 p10_no_inc_no_dec
done
# increase, no decrease
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 10 1 0 p10_inc_no_dec
done
# increase, decrease
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 10 1 1 p10_inc_dec
done

# starting packet 20
# no increase
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 20 0 0 p20_no_inc_no_dec
done
# increase, no decrease
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 20 1 0 p20_inc_no_dec
done
# increase, decrease
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 20 1 1 p20_inc_dec
done

# starting packet 40
# no increase
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 40 0 0 p40_no_inc_no_dec
done
# increase, no decrease
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 40 1 0 p40_inc_no_dec
done
# increase, decrease
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 40 1 1 p40_inc_dec
done

# starting packet 80
# no increase
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 80 0 0 p80_no_inc_no_dec
done
# increase, no decrease
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 80 1 0 p80_inc_no_dec
done
# increase, decrease
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 80 1 1 p80_inc_dec
done

# starting packet 160
# no increase
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 160 0 0 p160_no_inc_no_dec
done
# increase, no decrease
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 160 1 0 p160_inc_no_dec
done
# increase, decrease
for run in 01 02 04 08 16 32 64
do
  sbatch run$run.js 160 1 1 p160_inc_dec
done
