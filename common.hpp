#pragma once

#include <chrono>
#include <thread>
#include <vector>
#include <mpi.h>
#include <boost/filesystem.hpp>
#include "logging.hpp"

namespace mp {

enum ParticipantType {
  A,
  B
};

enum CommunicatorType {
  single,
  many
};

enum PublishingType {
  file,
  server
};

void sleep(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}


int getCommSize(MPI_Comm comm = MPI_COMM_WORLD)
{
  int size = -1;
  MPI_Comm_size(comm, &size);
  return size;
}

int getRemoteCommSize(MPI_Comm comm)
{
  int size = -1;
  MPI_Comm_remote_size(comm, &size);
  return size;
}

int getCommRank(MPI_Comm comm = MPI_COMM_WORLD)
{
  int rank = -1;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  return rank;
}

void removeDir(boost::filesystem::path path)
{
  if (getCommRank() == 0) {
    boost::filesystem::remove_all(path);
  }
  MPI_Barrier(MPI_COMM_WORLD);
}

/// Aquires a port name from MPI
std::string openPort()
{
  std::string p = std::string(MPI_MAX_PORT_NAME, '\0');
  MPI_Open_port(MPI_INFO_NULL, const_cast<char *>(p.data()));
  DEBUG << "Opened port: " << p;
  return p;
}

/// Writes port to file
void writePort(boost::filesystem::path path, std::string const & portName)
{
  DEBUG << "Writing portname " << portName << " to " << path;
  create_directory(path.parent_path());
  std::ofstream ofs(path.string(), std::ofstream::out);
  ofs << portName;
  ofs.flush();
}

/// Writes port to nameserver
void writePort(std::string const & serviceName, std::string const & portName)
{
  DEBUG << "Publishing portname " << portName << " as name " << serviceName;
  MPI_Publish_name(serviceName.c_str(), MPI_INFO_NULL, portName.c_str());
}

/// Reads port from a file
std::string readPort(boost::filesystem::path path)
{
  std::ifstream ifs;
  do  {
    ifs.open(path.string(), std::ifstream::in);
    std::this_thread::yield();
  } while (not ifs);
  // sleep(10);
  std::string portName;
  ifs >> portName;
  std::getline(ifs, portName);
  if (portName == "")
    ERROR << "READ EMPTY PORT NAME FROM " << path.string();
  portName.resize(MPI_MAX_PORT_NAME, '\0');
  DEBUG << "Read address " << portName << " from " << path;
  return portName;
}

/// Reads port from a nameserver
std::string readPort(std::string const & name)
{
  char p[MPI_MAX_PORT_NAME];
  DEBUG << "Looking up address at service name " << name;
  MPI_Lookup_name(name.c_str(), MPI_INFO_NULL, p);
  DEBUG << "Looked up address " << p;
  return p;
}

/// Ranks to connect to
/*
 * @param[in] peers Number of peers
 * @param[in] size Size of participant we connect to
 * @param[in] rank My own rank
 * @returns Sorted list of ranks
 */
std::vector<int> getRanks(int peers, int size, int rank)
{
  std::vector<int> ranks;
  ranks.push_back(rank);
  int acc1 = rank, acc2 = rank;
  for (int i = 0; i < peers; ++i) {
    if (acc1 > 0)
      ranks.push_back(--acc1);
    if (acc2 < size - 1)
      ranks.push_back(++acc2);
  }
  ranks.resize( std::min(peers, size) );
  std::sort(ranks.begin(), ranks.end());
  return ranks;
}

/// Ranks to connect to
/*
 * @param[in] peers Ratio or number of peers to connect
 * @param[in] size Size of participant we connect to
 * @param[in] rank My own rank
 * @returns Sorted list of ranks
 */
std::vector<int> getRanks(double peers, int size, int rank)
{
  if (peers < 1)
    return getRanks(static_cast<int>(std::round(size * peers)), size, rank);
  else
    return getRanks(static_cast<int>(peers), size, rank);
}

/// Inverts getRanks, i.e. who connects to me (rank)?
std::vector<int> invertGetRanks(double peers, int size, int rank)
{
  std::vector<int> igr;
  for (int r = 0; r < size; ++r) {
      auto rs = getRanks(peers, size, r);
      if (std::find(rs.begin(), rs.end(), rank) != rs.end())
        igr.push_back(r);
  }
  return igr;
}



/// Creates an intercom on rank 0 that can be used to synchronize both particpants
MPI_Comm createSyncIcomm(ParticipantType p, boost::filesystem::path publishDirectory)
{
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank != 0)
    return MPI_COMM_NULL;

  MPI_Comm comm;
  if (p == A) {
    auto port = openPort();
    writePort(publishDirectory / "sync-intercomm.address", port);
    MPI_Comm_accept(port.c_str(), MPI_INFO_NULL, 0, MPI_COMM_SELF, &comm);
  }
  if (p == B) {
    sleep(100);
    auto port = readPort(publishDirectory / "sync-intercomm.address");
    MPI_Comm_connect(port.c_str(), MPI_INFO_NULL, 0, MPI_COMM_SELF, &comm);
  }
  DEBUG << "Created sync intercomm";
  return comm;
}

}
