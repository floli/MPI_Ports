#include "prettyprint.hpp"
#include <iostream>
#include <mpi.h>
#include "logging.hpp"

/// Tests the amount of data that can be send, without a matching receive.
void send()
{
  double sizeofint = sizeof(int) / 1024.0;
  INFO << "sizeof(int) = " << sizeof(int);
  int i = 0;
  while (true) {
    INFO << "Iteration " << i << " data send = " << i * sizeofint << " KB";
    INFO << sizeofint;
    MPI_Send(&i, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
    ++i;
  }
}

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
  logging::init();

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0) {
    send();
  }
  
  MPI_Finalize();
}
