#include <mpi.h>
#include <thread>
#include <iostream>
#include <fstream>

using std::cout;
using std::endl;

/// Aquires a port name from MPI
std::string openPort()
{
  std::string p = std::string(MPI_MAX_PORT_NAME, '\0');
  MPI_Open_port(MPI_INFO_NULL, const_cast<char *>(p.data()));
  cout << "Opened port: " << p << endl;
  return p;
}

/// Writes port to file
void writePort(std::string filename, std::string const & portName)
{
  cout << "Writing portname " << portName << " to " << filename << endl;
  std::ofstream ofs(filename, std::ofstream::out);
  ofs << portName;
  ofs.flush();
}

/// Reads port from a file
std::string readPort(std::string filename)
{
  std::ifstream ifs;
  do  {
    ifs.open(filename, std::ifstream::in);
    std::this_thread::yield();
  } while (not ifs);
  // sleep(10);
  std::string portName;
  ifs >> portName;
  std::getline(ifs, portName);
  if (portName == "")
    cout << "READ EMPTY PORT NAME FROM " << filename << endl;
  cout << "Read address " << portName << " from " << filename;
  portName.resize(MPI_MAX_PORT_NAME, '\0');
  return portName;
}



int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
  int rank = -1;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  std::string participant = argv[1];

  std::string portName;
  // Open and write port
  if (participant == "A" and rank == 0) {
    portName = openPort();
    writePort("TEST_PORT", portName);
  }
  
  if (participant == "B" and rank == 0) {
    portName = readPort("TEST_PORT");
  }

  MPI_Comm icomm;
  // Accept / Connect
  if (participant == "A") {
    MPI_Comm_accept(portName.c_str(), MPI_INFO_NULL, 0, MPI_COMM_WORLD, &icomm);
    cout << "Accepted connection" << endl;
  }
  if (participant == "B") {
    MPI_Comm_connect(portName.c_str(), MPI_INFO_NULL, 0, MPI_COMM_WORLD, &icomm);
    cout << "Connected" << endl;      
  }

  if (participant == "A") {
    int i = 1;
    MPI_Send(&i, 1, MPI_INT, rank, 0, icomm);
  }
  if (participant == "B") {
    int i;
    MPI_Recv(&i, 1, MPI_INT, rank, MPI_ANY_TAG, icomm, MPI_STATUS_IGNORE);
  }
  
  MPI_Finalize();  
}
