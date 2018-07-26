#include <mpi.h>
#include <fstream>
#include <numeric>
#include <boost/filesystem.hpp>
#include "common.hpp"
#include "EventTimings/EventTimings.hpp"
#include "programoptions.hpp"
#include "logging.hpp"
#include "prettyprint.hpp"


/// Writes a port name to file or nameserver
void publishPort(Options options, std::string const & portName)
{
  using namespace boost::filesystem;
  int rank = getCommRank();

  if (options.commType == many and options.pubType == file)
    writePort(options.publishDirectory / ("A-" + std::to_string(rank) + ".address"), portName);
  if (options.commType == single and options.pubType == file)
    writePort(options.publishDirectory / "intercomm.address", portName);
  if (options.pubType == server)
    writePort(std::string("mpiports"), portName);
}

/// Reads a port for connection to `remoteRank` from file or nameserver
std::string lookupPort(Options options, int remoteRank = -1)
{
  std::string portName;
  if (options.commType == many and options.pubType == file) {
    assert(remoteRank >= 0);
    portName = readPort(options.publishDirectory / ( "A-" + std::to_string(remoteRank) + ".address"));
  if (options.commType == single and options.pubType == file)
    portName = readPort(options.publishDirectory / "intercomm.address");
  if (options.pubType == server)
    portName = readPort(std::string("mpiports"));

  return portName;
}


int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
  auto options = getOptions(argc, argv);
  logging::init(options.debug);
  // MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
  EventRegistry::instance().initialize(options.participant == A ? "A" : "B");

  int rank = getCommRank();
  int size = getCommSize();

  // ==============================

  Event _determineNumCon("Compute connections", true);
  std::vector<int> comRanks;
  if (options.participant == A)
    comRanks = invertGetRanks(options.peers, size, rank); // who connects to me?
  else if (options.participant == B)
    comRanks = getRanks(options.peers, size, rank); // to whom I do connect?
    
  _determineNumCon.stop();

  // ==============================
  if (options.participant == A)
    removeDir(options.publishDirectory); // Remove directory, followed by a barrier

  Event _publish("Publish", true);
  if (options.participant == A) {
    if (options.commType == single and rank == 0)
      publishPort(options, openPort());

    if (options.commType == many)
      for (auto r : comRanks)
        publishPort(options, openPort());
  }
  _publish.stop(); // Barrier
  // ==============================

  std::map<int, MPI_Comm> comms;
  Event _connect("Connect", true);
  std::string portName;
  if (options.participant == A) { // receives connections
    if (options.commType == single) {
      if (rank == 0)
        portName = lookupPort(options);
      INFO << "Accepting connection on " << portName;
      MPI_Comm_accept(portName.c_str(), MPI_INFO_NULL, 0, MPI_COMM_WORLD, &comms[0]);
      DEBUG << "Received connection on " << portName;
    }

    if (options.commType == many) {
      for (auto r : comRanks) {
        MPI_Comm icomm;
        portName = lookupPort(options, r);
        INFO << "Accepting connection on " << portName;
        MPI_Comm_accept(portName.c_str(), MPI_INFO_NULL, 0, MPI_COMM_SELF, &icomm);
        int connectedRank = -1;
        MPI_Recv(&connectedRank, 1, MPI_INT, 0, MPI_ANY_TAG, icomm, MPI_STATUS_IGNORE);
        comms[connectedRank] = icomm;
        MPI_Send(&rank, 1, MPI_INT, 0, 0, icomm);
      }
    }   
  }

  if (options.participant == B) { // connects to the intercomms
    if (options.commType == single) {
      if (rank == 0)
        portName = lookupPort(options);
      INFO << "Connecting to " << portName;
      MPI_Comm_connect(portName.c_str(), MPI_INFO_NULL, 0, MPI_COMM_WORLD, &comms[0]);
      DEBUG << "Connected to " << portName;
    }
    if (options.commType == many) {
      for (auto r : comRanks) {
        MPI_Comm icomm;
        portName = lookupPort(options, r);
        INFO << "Connecting to " << portName;
        MPI_Comm_connect(portName.c_str(), MPI_INFO_NULL, 0, MPI_COMM_SELF, &icomm);
        MPI_Send(&rank, 1, MPI_INT, 0, 0, icomm);
        int connectedRank = -1;
        MPI_Recv(&connectedRank, 1, MPI_INT, 0, MPI_ANY_TAG, icomm, MPI_STATUS_IGNORE);
        comms[connectedRank] = icomm;        
      }
    }
  }
  _connect.stop();

  // ==============================
  std::vector<double> dataVec(1000);
  std::iota(dataVec.begin(), dataVec.end(), 0.5);
  Event _dataexchange("Data Send/Recv", true);
  for (auto r : comRanks) {
    int actualRank;
    MPI_Comm actualComm;
    if (options.commType == single) {
      actualRank = r;
      actualComm = comms[0];
    }
    if (options.commType == many) {
      actualRank = 0;
      actualComm = comms[rank];
    }
    if (options.participant == A) {
      DEBUG << "Receiving data from " << actualRank; 
      MPI_Recv(dataVec.data(), dataVec.size(), MPI_DOUBLE, actualRank, MPI_ANY_TAG, actualComm, MPI_STATUS_IGNORE);
    }
    if (options.participant == B)
      DEBUG << "Sending data to " << actualRank; 
      MPI_Send(dataVec.data(), dataVec.size(), MPI_DOUBLE, actualRank, 0, actualComm);
  }

  _dataexchange.stop();

  EventRegistry::instance().finalize();
  EventRegistry::instance().printAll();
  MPI_Finalize();

  return 0;
}
