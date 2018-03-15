#!/bin/bash
#SBATCH --job-name="MPI_Ports"
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=8
#SBATCH --time=00:00:10
#SBATCH --mail-type=ALL

echo $SLURM_HOSTFILE
cat $SLURM_HOSTFILE

mpirun.mpich -n 4 ./mpiports  -p A -c single -m file > out.A &
mpirun.mpich -n 4 ./mpiports  -p B -c single -m file > out.B

wait
