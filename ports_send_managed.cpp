#include <mpi.h>
#include <memory>
#include "common.hpp"
#include "buffer_manager.hpp"

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
    MPI_Request req;
    auto sb = std::unique_ptr<double>(new double);
    *sb = i;
    MPI_Isend(sb.get(), 1, MPI_DOUBLE, 0, 0, comm, &req);
    auto reqPtr = std::unique_ptr<Request>(new Request(req));
    BufferManager::instance().put(std::move(reqPtr), std::move(sb));
    // MPI_Wait(&req, MPI_STATUS_IGNORE);
    // sleep(100);
    ++i;
  }
}

/// Delayed receive
void Receive(MPI_Comm comm)
{
  double sizeofint = sizeof(double) / 1024.0;
  INFO << "sizeof(int) = " << sizeof(double);
  double i = 0;
  while (true) {
    MPI_Recv(&i, 1, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, MPI_STATUS_IGNORE);
    INFO << "Received " << i << ", data received = " << i * sizeofint << " KB";
    sleep(1000);
  }
}

}

/// Launch two mpirun -n 1 processes
int main(int argc, char **argv)
{
  using namespace mp;
  // MPI_Init(&argc, &argv);
  int required = MPI_THREAD_MULTIPLE;
  int provided;
  MPI_Init_thread(&argc, &argv, required, &provided);
  assert(required == provided);
  logging::init(true);

  std::string participant = argv[1];

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);  

  MPI_Comm icomm;
  if (participant == "A") {
    auto & bm = BufferManager::instance();
    bm.run();
    icomm = accept();
    Isend(icomm);
    // Send(icomm);    
  }
  if (participant == "B") {
    icomm = request();
    Receive(icomm);
  }

  
  MPI_Finalize();
}
