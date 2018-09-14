#include <mpi.h>
#include <fstream>
#include <numeric>
#include <boost/filesystem.hpp>
#include "common.hpp"
#include "EventTimings/EventTimings.hpp"
#include "programoptions.hpp"
#include "logging.hpp"
#include "prettyprint.hpp"

namespace mp {

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
  }
  if (options.commType == single and options.pubType == file)
    portName = readPort(options.publishDirectory / "intercomm.address");
  if (options.pubType == server)
    portName = readPort(std::string("mpiports"));

  return portName;
}

}

int main(int argc, char **argv)
{
  using namespace mp;
  MPI_Init(&argc, &argv);
  auto options = getOptions(argc, argv);
  logging::init(options.debug);
  // MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

  int rank = getCommRank();
  int size = getCommSize();

  auto syncComm = createSyncIcomm(options.participant, options.publishDirectory); // First barrier
  EventRegistry::instance().initialize(options.participant == A ? "A" : "B", options.runName);

  std::map<int, MPI_Comm> comms;
  
  // ==============================
  Event _determineNumCon("Compute connections", true);
  std::vector<int> comRanks;
  if (options.participant == A)
    comRanks = invertGetRanks(options.peers, size, rank); // who connects to me?
  else if (options.participant == B)
    comRanks = getRanks(options.peers, size, rank); // to whom I do connect?

  _determineNumCon.stop(true);
  // ==============================
  
  if (options.participant == A)
    removeDir(options.publishDirectory); // Remove directory, followed by a barrier

  // ==============================
  Event _publish("Publish", true);
  if (options.participant == A) {
    if (options.commType == single and rank == 0)
      publishPort(options, openPort());

    if (options.commType == many)
      for (auto r : comRanks)
        publishPort(options, openPort());
  }
  _publish.stop(true);
  // ==============================

  DEBUG << "Finished publishing";
  if (rank == 0)
    MPI_Barrier(syncComm);

  INFO << "Starting connecting on " << size << " ranks.";
  Event _connect("Connect", true);
  std::string portName;
  if (options.participant == A) { // receives connections
    if (options.commType == single) {
      if (rank == 0)
        portName = lookupPort(options);
      DEBUG << "Accepting connection on " << portName;
      MPI_Comm_accept(portName.c_str(), MPI_INFO_NULL, 0, MPI_COMM_WORLD, &comms[0]);
      DEBUG << "Received connection on " << portName;
    }

    if (options.commType == many) {
      portName = lookupPort(options, rank);
      
      for (auto r : comRanks) {
        MPI_Comm icomm;
        DEBUG << "Accepting connection on " << portName;
        MPI_Comm_accept(portName.c_str(), MPI_INFO_NULL, 0, MPI_COMM_SELF, &icomm);
        DEBUG << "Accepted connection on " << portName;
        DEBUG << "icomm size = " << getRemoteCommSize(icomm);
        int connectedRank = -1;
        MPI_Recv(&connectedRank, 1, MPI_INT, 0, MPI_ANY_TAG, icomm, MPI_STATUS_IGNORE);
        // MPI_Send(&rank, 1, MPI_INT, 0, 0, icomm);
        DEBUG << "Received rank number " << connectedRank;
        comms[connectedRank] = icomm;
      }
    }
  }

  if (options.participant == B) { // connects to the intercomms
    // sleep(1000);
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
        INFO << "Connecting to rank " << r << " on " << portName;
        MPI_Comm_connect(portName.c_str(), MPI_INFO_NULL, 0, MPI_COMM_SELF, &icomm);
        DEBUG << "icomm size = " << getRemoteCommSize(icomm);
        DEBUG << "Connected to rank " << r << " on " << portName;
        MPI_Send(&rank, 1, MPI_INT, 0, 0, icomm);
        // int connectedRank = -1;
        // MPI_Recv(&connectedRank, 1, MPI_INT, 0, MPI_ANY_TAG, icomm, MPI_STATUS_IGNORE);
        comms[r] = icomm;
      }
    }
    // for (auto &c : comms)
    //   DEBUG << "Number of remote ranks = " << getRemoteCommSize(c.second);
  }
  _connect.stop(true);

  // ==============================

  INFO << "Starting data exchange";
  for (auto vecSize : {500, 2000, 4000}) {
    std::vector<double> dataVec(vecSize);
    std::iota(dataVec.begin(), dataVec.end(), 0.5);

    if (rank == 0)
      MPI_Barrier(syncComm);
    Event _dataexchange("Data " + std::to_string(vecSize), true);
    for (int round = 0; round < options.rounds; ++round) {
      for (auto r : comRanks) {
        int actualRank;
        MPI_Comm actualComm;
        if (options.commType == single) {
          actualRank = r;
          actualComm = comms[0];
        }
        if (options.commType == many) {
          actualRank = 0;
          actualComm = comms[r];
        }
        if (options.participant == A) {
          MPI_Recv(dataVec.data(), dataVec.size(), MPI_DOUBLE, actualRank, MPI_ANY_TAG, actualComm, MPI_STATUS_IGNORE);
          // MPI_Send(dataVec.data(), dataVec.size(), MPI_DOUBLE, actualRank, 0, actualComm);
        }
        if (options.participant == B) {
          MPI_Send(dataVec.data(), dataVec.size(), MPI_DOUBLE, actualRank, 0, actualComm);
          // MPI_Recv(dataVec.data(), dataVec.size(), MPI_DOUBLE, actualRank, MPI_ANY_TAG, actualComm, MPI_STATUS_IGNORE);
        }
      }
    }
    _dataexchange.stop(true);
  }

  
  EventRegistry::instance().finalize();
  EventRegistry::instance().printAll();
  MPI_Finalize();

  return 0;
}
