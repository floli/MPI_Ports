#pragma once
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>


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


struct Options
{
  ParticipantType participant;
  CommunicatorType commType;
  PublishingType pubType;
  boost::filesystem::path publishDirectory;
  double peers;

  explicit Options(boost::program_options::variables_map opt) {
    participant = opt["participant"].as<std::string>() == "A" ? A : B;
    publishDirectory = boost::filesystem::path(opt["publishDirectory"].as<std::string>());
    commType = opt["commType"].as<std::string>() == "single" ? single : many;
    pubType = opt["publishingType"].as<std::string>() == "file" ? file : server;
    peers = opt["peers"].as<double>();
  }
};


Options getOptions(int argc, char *argv[])
{
  namespace po = boost::program_options;
  po::options_description desc("mpiports: Benchmal MPI perfomance");
  desc.add_options()
    ("help,h", "produce help")
    ("participant,p", po::value<std::string>()->required(), "Participant Name, A or B")
    ("peers", po::value<double>()->default_value(4), "Peers to connect, if value < 1 it is interpreted as a ratio, if > 1, it is an absolute number")
    ("publishDirectory,d", po::value<std::string>()->default_value("./publish"), "Directory to publish connection information to")
    ("commType,c", po::value<std::string>()->required(), "Intercom type: 'single' or 'many'")
    ("publishingType,m", po::value<std::string>()->required(), "Publishing type: 'file' or 'server'");
  
  po::variables_map vm;        

  try {
    po::store(parse_command_line(argc, argv, desc), vm);
    
    if (vm.count("help")) {
      std::cout << desc << std::endl << std::endl;
      std::exit(-1);
    }
    po::notify(vm);
  }
  catch(po::error& e) {
    std::cout << "ERROR: " << e.what() << "\n\n";
    std::cout << desc << std::endl;
    std::exit(-1);
  }
  return Options(vm);
}
