#!/bin/bash
for start in 2 10 25 50 100
do
  # no increase
  for run in 01 02 04 08 16 32 64
  do
    sbatch --output=/dev/null --job-name="run$run-p$start-no_inc" run$run.js $start 0 p1_no_inc $1
  done
  # increase
  for run in 01 02 04 08 16 32 64
  do
    sbatch --output=/dev/null --job-name="run$run-p$start-inc" run$run.js $start 1 p1_inc $1
  done
done