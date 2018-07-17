#pragma once

#include <chrono>
#include <thread>
#include <vector>
#include <mpi.h>
#include <boost/filesystem.hpp>
#include "logging.hpp"


/// Aquires a port name from MPI
std::string openPort()
{
  char p[MPI_MAX_PORT_NAME];
  MPI_Open_port(MPI_INFO_NULL, p);
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
  std::ifstream ifs(path.string(), std::ifstream::in);
  std::string portName;
  ifs >> portName;
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
    if (acc2 < size)
      ranks.push_back(++acc2);
  }
  ranks.resize( std::min(peers, size) );
  std::sort(ranks.begin(), ranks.end());
  return ranks;
}

/// Ranks to connect to
/*
 * @param[in] ratio of peers
 * @param[in] size Size of participant we connect to
 * @param[in] rank My own rank
 * @returns Sorted list of ranks
 */
std::vector<int> getRanks(double ratio, int size, int rank)
{
  return getRanks(static_cast<int>(std::round(size * ratio)), size, rank);
}



void sleep(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
