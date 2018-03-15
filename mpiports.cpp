#include <mpi.h>
#include <fstream>
#include <numeric>
#include <boost/filesystem.hpp>
#include "EventTimings/EventTimings.hpp"
#include "programoptions.hpp"
#include "logging.hpp"
#include "prettyprint.hpp"

std::string openPort()
{
  char p[MPI_MAX_PORT_NAME];
  MPI_Open_port(MPI_INFO_NULL, p);
  DEBUG << "Opened port: " << p;
  return p;
}

void writePort(boost::filesystem::path path, std::string const & portName)
{
  DEBUG << "Writing portname " << portName << " to " << path;
  create_directory(path.parent_path());
  std::ofstream ofs(path.string(), std::ofstream::out);
  ofs << portName;
}

void writePort(std::string const & serviceName, std::string const & portName)
{
  DEBUG << "Publishing portname " << portName << " as name " << serviceName;
  MPI_Publish_name(serviceName.c_str(), MPI_INFO_NULL, portName.c_str());
}


void publishPort(Options options, std::string const & portName)
{
  using namespace boost::filesystem;
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (options.commType == many and options.pubType == file) 
    writePort(options.publishDirectory / ("A-" + std::to_string(rank) + ".address"), portName);
  if (options.commType == single and options.pubType == file)
    writePort(options.publishDirectory / "intercomm.address", portName);
  if (options.pubType == server)
    writePort(std::string("mpiports"), portName);
  
}

std::string readPort(boost::filesystem::path path)
{
  std::ifstream ifs(path.string(), std::ifstream::in);
  std::string portName;
  ifs >> portName;
  DEBUG << "Read address " << portName << " from " << path;
  return portName;
}

std::string readPort(std::string const & name)
{
  char p[MPI_MAX_PORT_NAME];
  DEBUG << "Looking up address at service name " << name;
  MPI_Lookup_name(name.c_str(), MPI_INFO_NULL, p);
  DEBUG << "Looked up address " << p;
  return p;
}


std::string lookupPort(Options options, int remoteRank = 0)
{  
  std::string portName;
  if (options.commType == many and options.pubType == file)
    portName = readPort(options.publishDirectory / ( "A-" + std::to_string(remoteRank) + ".address"));
  if (options.commType == single and options.pubType == file)
    portName = readPort(options.publishDirectory / "intercomm.address");
  if (options.pubType == server)
    portName = readPort(std::string("mpiports"));

  return portName;
}

std::vector<int> getRanks(double peers)
{
  int rank, size, max = 0, min = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  std::vector<int> ranks;
  if (peers > 1) {
    int numPeers = static_cast<int>(peers);
    // numPeers = rank + numPeers/2 > size ? numPeers 
    min = rank - numPeers / 2;
    max = rank + numPeers / 2;
    min = min < 0 ? 0 : min;
    max = max >= size ? size-1 : max;
    ranks.resize(max - min);
    std::iota(ranks.begin(), ranks.end(), min);
  }
  std::cout << "RANKS ON " << rank << " = " << ranks << std::endl;
  return ranks;
}



int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
  logging::init();
  // MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
  EventRegistry::instance().initialize();

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  auto options = getOptions(argc, argv);

  if (options.commType == many) {
    std::cout << "Many communicators are not supported as this time" << std::endl;
    return -1;
  }
  
  // getRanks(options.peers);
  // std::exit(0);

  // ==============================
  Event _publish("Publish", true); // works as an MPI_Barrier on start and stop
  if (options.participant == A) { // A publishes the port
    if (options.commType == single and rank == 0)
      publishPort(options, openPort());
 
    if (options.commType == many)
      publishPort(options, openPort());
  }
  _publish.stop(); // Barrier
  // ==============================

  Event _connect("Connect", true);
  MPI_Comm icomm;
  std::string portName;
  if (options.participant == A) { // receives connections
    if (options.commType == single) {
      if (rank == 0)
        portName = lookupPort(options);
      INFO << "Accepting connection on " << portName;
      MPI_Comm_accept(portName.c_str(), MPI_INFO_NULL, 0, MPI_COMM_WORLD, &icomm);
      INFO << "Received connection";
    }
  }

  if (options.participant == B) { // connects to the intercomms
    if (options.commType == single) {
      if (rank == 0)
        portName = lookupPort(options);
      INFO << "Trying to connect to " << portName;
      MPI_Comm_connect(portName.c_str(), MPI_INFO_NULL, 0, MPI_COMM_WORLD, &icomm);
      INFO << "Connected";
    }
  }
  _connect.stop();

  // size_t fillSize = 100;
  std::vector<double> dataVec(1000);
  std::iota(dataVec.begin(), dataVec.end(), 0.5);
  
  Event _dataexchange("Data Send/Recv", true);
  if (options.participant == A) {
    MPI_Send(dataVec.data(), dataVec.size(), MPI_DOUBLE, rank, 0, icomm);
  }
  if (options.participant == B) {
    MPI_Recv(dataVec.data(), dataVec.size(), MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, icomm, MPI_STATUS_IGNORE);
  }  
  _dataexchange.stop();  
  
  EventRegistry::instance().finalize();
  EventRegistry::instance().printAll();
  MPI_Finalize();
  
  return 0;
}
