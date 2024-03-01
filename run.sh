#!/bin/bash
for start in 1 2 10 25 50 100
do
  # no increase
  for run in 01 02 04 08 16 32 64
  do
    sbatch --output=/dev/null --job-name="run$run-p$start-no_inc-$2" run$run.js $start 0 p${start}_no_inc $1
  done
  # increase
  for run in 01 02 04 08 16 32 64
  do
    sbatch --output=/dev/null --job-name="run$run-p$start-inc-$2" run$run.js $start 1 p${start}_inc $1
  done
done