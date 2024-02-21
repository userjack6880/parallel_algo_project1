After login to ptolemy-login-1.arc.msstate.edu, you will need to login to the
head node of the development nodes with the command:

ssh -Y ptolemy-devel-1


Before you use mpi on the ptolemy system for the first time you will need
load in the mpi package which you can do with the commands

module load openmpi

If you want to save entering this commmand every time you log in, you
can edit your ~/.bashrc filel and add the above command just above the
line that reads:

#module load local

For editing text files, you can use the editors vi, nano, or emacs to
perform this editing.

*   nano is the easiest editor to use for the uninitiated.

Now you will need to copy the project1.tar file your work directory
with the command:

First change to your work directory on the cluster:

cd /work/ptolemy/class/cse4163/<netid>

Then copy the tar file to your work directory

cp /reference/ptolemy/class/cse4163/project1.tar .

To unpack this file just use the command:

tar xvf project1.tar

You will now have a directory called 'project1' that will contain the starting
source code for your project.  Enter the directory using the command 
'cd project1'


Then you can compile using the command "make" in the project directory.  This
assumes that you have performed the module load command discussed above.


This directory contains several program files:

game.h:      This file defines a structs that is used in solving a puzzle
game.cc:     Implementation of methods defined in game.h
utilities.h: Define utility routines that will measure time and kill runaway
             jobs.
utilities.cc:Implementation of utility routines
main.cc:     Program main and implementation of server and client code.

easy_sample.dat:  A sample of puzzles that are computationally easy to solve
                  (to be used for debugging program)
hard_sample.dat:  A sample of puzzles that are computationally hard to solve
                  (to be used for measuring program performance)

debug0?.js:  A selection of job scripts for debugging runs on the
             parallel cluster

run??.js:    A selection of job scripts for performance runs on the
             parallel cluster
-----------------------------------------------------------------------------

How to submit jobs to the parallel cluster using the Slurm batch system:

To submit a job once the program has been compiled use one of the
provided Slurm job scripts (these end in .js).  These job scripts have
been provided to run parallel jobs on the cluster.  To submit a job use 
the "sbatch" command. (note:  "man sbatch" to get detailed information on this 
command)

example:
sbatch debug01.js

To see the status of jobs on the queue, use squeue.  Example:

ptolemy-login-1[134] lush$ squeue -u lush
             JOBID PARTITION     NAME     USER ST       TIME  NODES NODELIST(REASON) 
             20298   48p160h  Work32P     lush PD       0:00      2 (ReqNodeNotAvail, UnavailableNodes:Shadow-1-[01-40],Shadow-2-[01-40],Shadow-3-[01-40],Shadow-4-[01-40]) 
             20297   48p160h  Work16P     lush PD       0:00      1 (ReqNodeNotAvail, UnavailableNodes:Shadow-1-[01-40],Shadow-2-[01-40],Shadow-3-[01-40],Shadow-4-[01-40]) 

This lists information associated with the job.  The important things to note
are the Job id and the state (ST).  The state tells the status of the job.  
Generally the status will be one of two values:  PD -  for pending or R for Running.  

Additionally, if you decide that you don't want to run a job, or it seems to
not work as expected, you can run "scancel Job id" to delete it from the queue.
For example, to remove the above job from the queue enter the command

scancel 20297
