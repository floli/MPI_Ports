#include <mpi.h>
#include "common.hpp"

namespace mp {

MPI_Comm accept()
{
  MPI_Comm icomm;
  std::string port = openPort();
  writePort("publish" / boost::filesystem::path("ports_send_buffer.address"), port);
  MPI_Comm_accept(port.c_str(), MPI_INFO_NULL, 0, MPI_COMM_WORLD, &icomm);
  INFO << "Sucessfully accepted connection.";
  return icomm;
}

MPI_Comm request()
{
  MPI_Comm icomm;
  std::string port = readPort("publish" / boost::filesystem::path("ports_send_buffer.address"));
  MPI_Comm_connect(port.c_str(), MPI_INFO_NULL, 0, MPI_COMM_WORLD, &icomm);
  INFO << "Sucessfully connected.";
  return icomm;
}

/// Tests the amount of data that can be send, without a matching receive.
void Send(MPI_Comm comm)
{
  double sizeofint = sizeof(int) / 1024.0;
  INFO << "sizeof(int) = " << sizeof(int);
  int i = 0;
  while (true) {
    INFO << "Iteration " << i << ", data send = " << i * sizeofint << " KB";
    MPI_Send(&i, 1, MPI_INT, 0, 0, comm);
    ++i;
  }
}

/// Tests the amount of data that can be send, without a matching receive.
void Isend(MPI_Comm comm)
{
  double sizeofint = sizeof(int) / 1024.0;
  INFO << "sizeof(int) = " << sizeof(int);
  int i = 0;
  while (true) {
    INFO << "Iteration " << i << ", data send = " << i * sizeofint << " KB";
    INFO << sizeofint;
    MPI_Request req;
    MPI_Isend(&i, 1, MPI_INT, 0, 0, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    ++i;
  }
}

}

/// Launch two mpirun -n 1 processes
int main(int argc, char **argv)
{
  using namespace mp;
  MPI_Init(&argc, &argv);
  logging::init(true);

  std::string participant = argv[1];

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  MPI_Comm icomm;
  if (participant == "A") {
    icomm = accept();
    Isend(icomm);
    Send(icomm);    
  }
  if (participant == "B") {
    icomm = request();
    sleep(1000 * 60);
  }

  
  MPI_Finalize();
}
