#include "prettyprint.hpp"
#include <iostream>
#include <mpi.h>
#include "logging.hpp"

/// Tests the amount of data that can be send, without a matching receive.
void Send()
{
  double sizeofint = sizeof(int) / 1024.0;
  INFO << "sizeof(int) = " << sizeof(int);
  int i = 0;
  while (true) {
    INFO << "Iteration " << i << ", data send = " << i * sizeofint << " KB";
    MPI_Send(&i, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
    ++i;
  }
}

/// Tests the amount of data that can be send, without a matching receive.
void Isend()
{
  double sizeofint = sizeof(int) / 1024.0;
  INFO << "sizeof(int) = " << sizeof(int);
  int i = 0;
  while (true) {
    INFO << "Iteration " << i << ", data send = " << i * sizeofint << " KB";
    MPI_Request req;
    MPI_Isend(&i, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
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
    Send();
  }
  
  MPI_Finalize();
}
