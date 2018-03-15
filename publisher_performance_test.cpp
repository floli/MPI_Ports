#include <boost/filesystem.hpp>
#include <iostream>
#include <mpi.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <chrono>
#include <thread>
#include <fstream>


//prefixDir with last char "/"
std::string lookUpPortFromFile(std::string filePath)
{
  std::ifstream ifs;

  do {
    ifs.open(filePath, std::ifstream::in);
  } while (not ifs);
  std::string dataString;
  std::getline(ifs, dataString);
  return dataString;
  // {
  //   std::istringstream iss(dataString);
  //   iss >> portName;
  // }
}

//prefixDir with last char "/"
void publishPortToFile(std::string const & filePath, std::string const & portName)
{
  using namespace boost::filesystem;
  create_directory(path(filePath).parent_path().string());
  {
    std::ofstream ofs(filePath + "~", std::ofstream::out);
    ofs << portName;
  }
  rename(filePath + "~", filePath);
}

//prefixDir with last char "/"
bool unPublishPortToFile(std::string filePath)
{
  return boost::filesystem::remove(filePath);
}

std::string lookUpPortNameServer(std::string serviceName)
{
  int lookupErr;
  char portName[MPI_MAX_PORT_NAME];
  do {
    lookupErr = MPI_Lookup_name(serviceName.c_str(), MPI_INFO_NULL, portName);
  } while (lookupErr);
  return std::string(portName);
}

void publishPortToNameServer(std::string const & serviceName, std::string const & portName)
{
  int err = MPI_Publish_name(serviceName.c_str(), MPI_INFO_NULL, portName.c_str());
  if (err) {
    char errmsg[MPI_MAX_ERROR_STRING];
    int msglen;
    MPI_Error_string(err, errmsg, &msglen);
    printf("Error in Publish_name: \"%s\"\n", errmsg);
    fflush(stdout);
  }
}

void unPublishPortToNameServer(std::string const & serviceName, std::string const & portName)
{
  int err = MPI_Unpublish_name(serviceName.c_str(), MPI_INFO_NULL, portName.c_str());
  if (err) {
    char errmsg[MPI_MAX_ERROR_STRING];
    int msglen;
    MPI_Error_string(err, errmsg, &msglen);
    printf("Error in Publish_name: \"%s\"\n", errmsg);
    fflush(stdout);
  }
}

void printHeaders(std::ofstream& ofs, const int communicator_type, const int num_connections, const char participant)
{
  ofs << "participant;";
  ofs << "num_rank;";
  ofs << "num_connections;";
  if (participant == 'A'){

    switch (communicator_type){
      case 0:{
        ofs << "filesystem_creation_time;" ;
        break;
      }
    } 
    ofs << "publish_time;";
    if (communicator_type < 3){
      for (int i = 0; i < num_connections ; ++i){
        ofs << "accept_times[" << std::to_string(i) << "];" ;
      }
    }
    ofs << "accept_time;";
    ofs << "unpublish_time;";
  } else {
    switch (communicator_type){
      case 0:
      case 1:{
        for (int i = 0; i < num_connections; ++i){
          ofs << "lookUpTime[" << std::to_string(i) << "];";
        }
        for (int i = 0; i < num_connections; ++i){
          ofs << "connect_times[" << std::to_string(i) << "];";
        }
        break;
      } 
    }
    ofs << "lookup_time;";
    ofs << "connect_time;";
  }
  ofs << "send_recv_time" << std::endl;
}

void printValuesParticipantA (std::ofstream& ofs,
                              const int communicator_type,
                              const int size,
                              const int rank,
                              const int num_connections, 
                              const double filesystem_creation_time, 
                              const double publish_time, 
                              double* accept_times, 
                              double& accept_time, 
                              const double unpublish_time, 
                              const double send_recv_time)
{
  ofs << "A;";
  ofs << std::to_string(size) << ";";
  ofs << std::to_string(num_connections) << ";";
  switch (communicator_type){
    case 0:{
      if (rank == 0){
        ofs << std::to_string(filesystem_creation_time);
      }
      ofs << ";";
      break;
    }
  }
  
  ofs << std::to_string(publish_time) << ";";
  if (communicator_type < 3){
    accept_time = 0;
    for (int i = 0; i < num_connections; ++i){
      ofs << std::to_string(accept_times[i]) << ";" ;
      accept_time += accept_times[i];
    }
  }
  ofs << std::to_string(accept_time) << ";";
  ofs << std::to_string(unpublish_time) << ";";
  ofs << std::to_string(send_recv_time) << std::endl;

}

void printValuesParticipantB(std::ofstream& ofs,
                             const int communicator_type,
                             const int size,
                             const int rank,
                             const int num_connections,
                             double* lookup_times,
                             double* connect_times,
                             double& lookup_time,
                             double& connect_time,
                             const double send_recv_time)
{ 
  ofs << "B;";
  ofs << std::to_string(size) << ";";
  ofs << std::to_string(num_connections) << ";";
  switch (communicator_type){
    case 0:
    case 1:{
      lookup_time = 0;
      connect_time = 0;
      for (int i = 0; i < num_connections; ++i){
        ofs << std::to_string(lookup_times[i]) << ";";
        lookup_time += lookup_times[i];
      }
      for (int i = 0; i < num_connections; ++i){
        ofs << std::to_string(connect_times[i]) << ";";
        connect_time += connect_times[i];
      }

      break;
    } 
  }
  if (communicator_type != 3 || rank == 0){ 
    ofs << std::to_string(lookup_time) << ";";
  } else {
    ofs << ";";
  }
  ofs << std::to_string(connect_time) << ";";
  ofs << std::to_string(send_recv_time) << std::endl;
}


void fillDataWithRandom(int* data, int arraySize)
{
  for (int i = 0; i < arraySize; i++) {
    data[i] = rand();
  }
}

int main(int argc, char** argv){

  if (argc != 8) {
    std::cout << std::endl;
    std::cout << "Usage: mpiexec -np <#procs> " << argv[0] << " <Participant> <communicator_type> <numconncetions> <rounds> <vectorsize> <exchange_dir> <results_dir>" << std::endl;
    std::cout << std::endl;
    std::cout << "Participant:     {A,B}." << std::endl;
    std::cout << "communicator_type:  0 = all nodes have their own comms publishing to files" << std::endl;
    std::cout << "                    1 = all nodes have their own comms publishing to name server" << std::endl;
    std::cout << "                    3 = one intercomm via filesystem" << std::endl;
    std::cout << "numconnections:     each rank establish (has to be even, < #procs)" << std::endl;
    std::cout << "rounds:             of rounds to test should be equal to both participants." << std::endl;
    std::cout << "vectorsize:         Size of vector that will be exchanged" << std::endl;
    std::cout << "exchange_dir:       directory for exchanging ports" << std::endl;
    std::cout << "results_dir:        directory for saving results" << std::endl;
    return -1;
  }

  MPI_Status status;

  MPI_Init(&argc, &argv);

  int rounds, vectorsize, rank, size;
  int communicator_type, num_connections;
  char participant;

  MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  
  participant = *argv[1];
  communicator_type = atoi(argv[2]);
  num_connections = atoi(argv[3]);
  rounds = atoi(argv[4]);
  vectorsize = atoi(argv[5]);
  std::string parent_dir(argv[6]);
  std::string results_dir(argv[7]);

  srand(4356789);
  int spread = (num_connections - 1)/2;
  int conn_pan = 0;
  //prevents dead locks by panning the connection order for the first spread count connections
  if (rank < spread){
    conn_pan = spread - rank;
  }
  
  MPI_Comm comms[num_connections];
  int comms_mapping[num_connections];
  
  //intercomm for communicator type 3
  MPI_Comm ic;
  //syncintercomm for performnce tests
  MPI_Comm sync_ic;
  //connect slaves with each other

  if (participant == 'A'){
  
    std::string publishDir = parent_dir;
    double filesystem_creation_time;
    if (communicator_type == 0){
      filesystem_creation_time = MPI_Wtime();
      if (rank == 0){
        for (int _rank = 0; _rank < size; ++_rank) {
          boost::filesystem::create_directory(parent_dir + "/" + ".B-A-" + std::to_string(_rank) + ".address");
        }
      }
      filesystem_creation_time = MPI_Wtime() - filesystem_creation_time;
      publishDir += "/.B-A-" + std::to_string(rank) + ".address";
    }
            
    MPI_Barrier(MPI_COMM_WORLD); 
    std::string fileServiceName = ".B-A-" + std::to_string(rank) + ".address";
    char port[MPI_MAX_PORT_NAME];
    switch (communicator_type){
      case 0:
      case 1:{
        MPI_Open_port(MPI_INFO_NULL, port);
        break;
      }
      case 3:{
        if (rank == 0){ 
          MPI_Open_port(MPI_INFO_NULL, port);
        }
      }
    }
    
    //time meassuring of publishing
    double publish_time = MPI_Wtime();
    
    switch (communicator_type){
      case 0:{
        publishPortToFile(publishDir + "/" + fileServiceName, port);
        break;
      }
      case 1:{
        publishPortToNameServer(fileServiceName, port);
        break;
      }
      case 3:{

#ifndef NDEBUG 
        std::cout << "(" << rank << ") has port= " << port << std::endl; 
#endif
        if (rank == 0){ 
          publishPortToFile(parent_dir + "/intercomm.address" , port);
        }
        break;
      }
    }
    publish_time = MPI_Wtime() - publish_time;


    //getting sync communicators
#ifndef NDEBUG 
    std::cout << "(" << rank << ") before barrier before getting sync comm " << std::endl; 
#endif
    
    MPI_Barrier(MPI_COMM_WORLD); 
#ifndef NDEBUG 
    std::cout << "(" << rank << ") after before barrier before getting sync comm" << std::endl; 
#endif


#ifndef NDEBUG 
    std::cout << "(" << rank << ") before sync comm accept" << std::endl; 
#endif
    
    MPI_Comm_accept(port, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &sync_ic);
#ifndef NDEBUG 
    std::cout << "(" << rank << ") after sync comm accept" << std::endl; 
#endif

    //time meassuring of accepting
    double accept_time;
    double accept_times[num_connections];
    switch (communicator_type){
      case 0:
      case 1:{
        MPI_Barrier(sync_ic);
    
        for (int i=0; i < num_connections; ++i) {
          int num_connection = (i + conn_pan + num_connections) % num_connections;
#ifndef NDEBUG 
          std::cout << "(" << rank << ") before comm barrier i=" << std::to_string(i) << " num_connection = " << std::to_string(num_connection) << std::endl; 
#endif
          MPI_Send(&rank, 1, MPI_INT, (rank - spread + num_connection + size) % size, 42, sync_ic);
          int syncvalue;
          MPI_Recv(&syncvalue, 1, MPI_INT, (rank - spread + num_connection + size) % size, 42, sync_ic , &status);
#ifndef NDEBUG 
          std::cout << "(" << rank << ") before accept num_connection=" << std::to_string(num_connection) << std::endl; 
#endif
          double acceptTimeBegin = MPI_Wtime();
          MPI_Comm_accept(port, MPI_INFO_NULL, 0, MPI_COMM_SELF, &comms[num_connection]);

          accept_times[num_connection] = MPI_Wtime() - acceptTimeBegin;
          int recv_remote_rank;
          MPI_Recv(&recv_remote_rank, 1, MPI_INT, 0, 42, comms[num_connection], &status);

#ifndef NDEBUG 
          std::cout << "(" << rank << ") i = " << std::to_string(i) << " num_connection = " << std::to_string(num_connection) << " received rank " << std::to_string(recv_remote_rank) << " expected rank= " << std::to_string((rank - spread + num_connection + size)% size) << " mapping value= " << std::to_string(recv_remote_rank+spread-rank) << std::endl; 
#endif
          comms_mapping[(recv_remote_rank+spread-rank + size) % size] = num_connection;

        }
        break;
      }
      case 3:{
        MPI_Barrier(sync_ic);
        accept_time = MPI_Wtime();

#ifndef NDEBUG 
        std::cout << "(" << rank << ") before accept" << std::endl; 
#endif

        MPI_Comm_accept(port, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &ic);
#ifndef NDEBUG 
        std::cout << "(" << rank << ") after accept" << std::endl; 
#endif
        accept_time = MPI_Wtime() - accept_time;
        break;
      }      
    }

#ifndef NDEBUG 
    std::cout << "(" << rank << ") before unpublishing barrier" << std::endl; 
#endif
    //unpublishing
    double unpublish_time_start = MPI_Wtime();
    switch (communicator_type){
      case 0:{
        MPI_Barrier(MPI_COMM_WORLD);
        unPublishPortToFile(publishDir  + "/" + fileServiceName);
        boost::filesystem::remove(publishDir);
        break;
      }
      case 1:{
        
        //TODO Bug ? example error
        //[mpiexec@i19r01c23] HYD_pmcd_pmi_unpublish (../../pm/pmiserv/pmiserv_pmi.c:247): assert (!closed) failed
        //[mpiexec@i19r01c23] fn_unpublish_name (../../pm/pmiserv/pmiserv_pmi_v1.c:1225): error unpublishing service
        //[mpiexec@i19r01c23] handle_pmi_cmd (../../pm/pmiserv/pmiserv_cb.c:69): PMI handler returned error
        //[mpiexec@i19r01c23] control_cb (../../pm/pmiserv/pmiserv_cb.c:958): unable to process PMI command
        //[mpiexec@i19r01c23] HYDT_dmxu_poll_wait_for_event (../../tools/demux/demux_poll.c:76): callback returned error status
        //[mpiexec@i19r01c23] HYD_pmci_wait_for_completion (../../pm/pmiserv/pmiserv_pmci.c:500): error waiting for event
        //[mpiexec@i19r01c23] main (../../ui/mpich/mpiexec.c:1130): process manager error waiting for completion
        //MPI_Barrier(MPI_COMM_WORLD);
        //3 ranks are working
        //unPublishPortToNameServer(fileServiceName, port);
        
        break;
      }
      case 3:{
        unPublishPortToFile(parent_dir + "/intercomm.address");
        break;
      }
    }
    double unpublish_time = MPI_Wtime() - unpublish_time_start;
    //MPI_Close_port(port);

    //time meassuring of sending nd receiving
#ifndef NDEBUG 
    std::cout << "(" << rank << ") before sending receive" << std::endl; 
#endif
    MPI_Barrier(sync_ic);
    double send_recv_time = MPI_Wtime();
    switch (communicator_type){
      case 0:
      case 1:{
        for (int i = 0; i < rounds; i++) {
          int sendData[vectorsize];
          int recvData[vectorsize];
#ifndef NDEBUG
          std::cout << "(" << rank << ") before fillData i= " << std::to_string(i) << std::endl;
#endif
          fillDataWithRandom(sendData, vectorsize);
          for (int j=0; j < num_connections; ++j) {
            int num_connection = (j + conn_pan) % num_connections;
#ifndef NDEBUG
            std::cout << "(" << rank << ") before send i= " << std::to_string(i) << " num_connection= " << std::to_string(num_connection) << std::endl;
#endif
            MPI_Recv(&recvData, vectorsize, MPI_INT, 0, 42, comms[comms_mapping[num_connection]], &status);
#ifndef NDEBUG
            std::cout << "(" << rank << ") before recv i= " << std::to_string(i) << " num_connection= " << std::to_string(num_connection) << std::endl;
#endif
            MPI_Send(&sendData, vectorsize, MPI_INT, 0, 42, comms[comms_mapping[num_connection]]);
#ifndef NDEBUG
            std::cout << "(" << rank << ") after recv i= " << std::to_string(i) << " num_connection= " << std::to_string(num_connection) << std::endl;
#endif
          }    
        }
        break;
      }
      case 3:{
        if (communicator_type == 3){
          for (int i = 0; i < rounds; i++) {
            int sendData[vectorsize];
            int recvData[vectorsize];
            fillDataWithRandom(sendData, vectorsize);
            for (int j=0; j < num_connections; ++j) {
              int num_connection = (j + conn_pan + num_connections) % num_connections;
              int remote_rank = (rank - spread + num_connection + size) % size;
              MPI_Recv(&recvData, vectorsize, MPI_INT, remote_rank , 42, ic, &status);
              MPI_Send(&sendData, vectorsize, MPI_INT, remote_rank, 42, ic);
            }    
          }
        }
        break;
      }
    }
    send_recv_time = MPI_Wtime() - send_recv_time;

#ifdef NDEBUG      
    std::string timesFileName = results_dir + "/a/n-"+ std::to_string(size) + "_rank-" + std::to_string(rank) +"_participant-a_times_num_connections-" + std::to_string(num_connections) + "_communicator_type-" + std::to_string(communicator_type);
    boost::filesystem::create_directory(boost::filesystem::path(timesFileName).parent_path().string());
    std::ofstream ofs(timesFileName, std::ofstream::out);
    //printing headers only in rank 0 file
    if (rank == 0){
      printHeaders(ofs, communicator_type, num_connections, participant);
    }
    //printing collected Values
    printValuesParticipantA(ofs, communicator_type, size, rank, num_connections, filesystem_creation_time, publish_time, accept_times, accept_time, unpublish_time, send_recv_time);

    ofs.close();
#endif  
        
    
    //participantB  
  }
  else {
    //getting sync communicators
    std::string port;
#ifndef NDEBUG 
    std::cout << "(" << rank << ") before sync comm lookup" << std::endl; 
#endif
    if (rank == 0){
      std::string publishDir = parent_dir + "/.B-A-0.address";
      std::string fileServiceName = ".B-A-0.address";

      switch (communicator_type){
        case 0: {
          port = lookUpPortFromFile(publishDir + "/" + fileServiceName);
          break;
        }
        case 1: {
          port = lookUpPortNameServer(fileServiceName);
          break;
        }
        case 3: {
          port = lookUpPortFromFile(parent_dir + "/intercomm.address");
          break;
        }
      }
#ifndef NDEBUG 
      std::cout << "(" << rank << ") looked up sync comm address= " << port << std::endl; 
#endif
    }

    MPI_Comm_connect(port.c_str(), MPI_INFO_NULL, 0, MPI_COMM_WORLD, &sync_ic);
#ifndef NDEBUG 
    std::cout << "(" << rank << ") after sync comm connect" << std::endl; 
#endif
            
  
    //time meassuring of lookup and connecting
    double lookup_times[num_connections];
    double connect_times[num_connections];
    double lookup_time;
    double connect_time;
    switch (communicator_type){
      case 0:
      case 1:{
        MPI_Barrier(sync_ic);
        for (int i=0; i < num_connections; ++i) {
          int num_connection = (i + conn_pan + num_connections) % num_connections;
          int remote_rank = (rank - spread + num_connection + size) % size;
          std::string publishDir = parent_dir + "/.B-A-" + std::to_string(remote_rank) + ".address";
          std::string fileServiceName = ".B-A-" + std::to_string(remote_rank) + ".address";
          std::string port;

#ifndef NDEBUG 
        std::cout << "(" << rank << ") before lookup" << std::endl; 
#endif          
          double lookup_time_start = MPI_Wtime();
          switch (communicator_type){
            case 0: {
              port = lookUpPortFromFile(publishDir + "/" + fileServiceName);
              break;
            }
            case 1: {
              port = lookUpPortNameServer(fileServiceName);
              break;
            }
          } 
          lookup_times[num_connection] = MPI_Wtime() - lookup_time_start;
        
#ifndef NDEBUG 
          std::cout << "(" << rank << ") before comm barrier num_connection=" << std::to_string(num_connection) << std::endl; 
#endif
          int syncvalue;
          MPI_Recv(&syncvalue, 1, MPI_INT, remote_rank, 42, sync_ic , &status);
          MPI_Send(&rank, 1, MPI_INT, remote_rank, 42, sync_ic);


#ifndef NDEBUG 
          std::cout << "(" << rank << ") before connect num_connection=" << std::to_string(num_connection) << std::endl; 
#endif
          double connect_time_start = MPI_Wtime();
          int connectErr;

          do {
            connectErr = MPI_Comm_connect(port.c_str(), MPI_INFO_NULL, 0, MPI_COMM_SELF, &comms[num_connection]);
            if (connectErr){
              std::cout << "(" << rank << ") num_connection = " << std::to_string(num_connection) << " connect Error" << std::endl;
            }
          } while (connectErr);
          connect_times[num_connection] = MPI_Wtime() - connect_time_start;
#ifndef NDEBUG 
          std::cout << "(" << rank << ") i=" << std::to_string(i) <<" send num_connection=  " << std::to_string(num_connection) << std::endl;
#endif 
          MPI_Send(&rank, 1, MPI_INT, 0, 42, comms[num_connection]);
        }
        break;  
      }
      case 3:{
        std::string port;
#ifndef NDEBUG 
        std::cout << "(" << rank << ") before lookup" << std::endl; 
#endif
        lookup_time = MPI_Wtime();
        if (rank == 0){
          port = lookUpPortFromFile(parent_dir + "/intercomm.address");
        }
        lookup_time = MPI_Wtime() - lookup_time;
#ifndef NDEBUG 
        std::cout << "(" << rank << ") looked up address= " << port << std::endl; 
#endif
        MPI_Barrier(sync_ic);
        connect_time = MPI_Wtime();
        MPI_Comm_connect(port.c_str(), MPI_INFO_NULL, 0, MPI_COMM_WORLD, &ic);
        connect_time = MPI_Wtime() - connect_time;
#ifndef NDEBUG 
        std::cout << "(" << rank << ") after connect" << std::endl; 
#endif
        break;
      }
    }
    
    //time meassuring of some sending and receiving
#ifndef NDEBUG 
      std::cout << "(" << rank << ") before send/receive " << std::endl;
#endif
    
    MPI_Barrier(sync_ic);
    double send_recv_time = MPI_Wtime();
    switch (communicator_type){
      case 0:
      case 1:{
        for (int i = 0; i < rounds; i++) {
          int sendData[vectorsize];
          int recvData[vectorsize];
          fillDataWithRandom(sendData, vectorsize);
#ifndef NDEBUG
          std::cout << "(" << rank << ") before fillData i= " << std::to_string(i) << std::endl;
#endif
          for (int j=0; j < num_connections; ++j) {
            int num_connection = (j + conn_pan + num_connections) % num_connections;
#ifndef NDEBUG
            std::cout << "(" << rank << ") before send i= " << std::to_string(i) << " num_connection= " << std::to_string(num_connection) << std::endl;
#endif
            MPI_Send(&sendData, vectorsize, MPI_INT, 0, 42, comms[num_connection]);
#ifndef NDEBUG
            std::cout << "(" << rank << ") before recv i= " << std::to_string(i) << " num_connection= " << std::to_string(num_connection) << std::endl;
#endif
            MPI_Recv(&recvData, vectorsize, MPI_INT, 0, 42, comms[num_connection], &status);
#ifndef NDEBUG
            std::cout << "(" << rank << ") after recv i= " << std::to_string(i) << " num_connection= " << std::to_string(num_connection) << std::endl;
#endif
          }    
        }
        break;
      }
      case 3:{
        for (int i = 0; i < rounds; i++) {
          int sendData[vectorsize];
          int recvData[vectorsize];
          fillDataWithRandom(sendData, vectorsize);

          for (int j = 0; j < num_connections; ++j) {
            int num_connection = (j + conn_pan + num_connections) % num_connections;
            int remote_rank = (rank - spread + num_connection + size) % size;
            MPI_Send(&sendData, vectorsize, MPI_INT, remote_rank, 42, ic);
            MPI_Recv(&recvData, vectorsize, MPI_INT, remote_rank, 42, ic, &status);
          }
        }
        break;
      }
    }
    send_recv_time = MPI_Wtime() - send_recv_time;
    

    
#ifdef NDEBUG    
    std::string timesFileName = results_dir + "/b/n-"+ std::to_string(size) + "_rank-" + std::to_string(rank) +"_participant-b_times_num_connections-" + std::to_string(num_connections) + "_communicator_type-" + std::to_string(communicator_type);
    boost::filesystem::create_directory(boost::filesystem::path(timesFileName).parent_path().string());
    std::ofstream ofs(timesFileName, std::ofstream::out);
    //printing headers only in rank 0 file
    if (rank == 0){
      printHeaders(ofs, communicator_type, num_connections, participant);
    }
    //printing collected Values
    printValuesParticipantB(ofs, communicator_type, size, rank, num_connections, lookup_times, connect_times, lookup_time, connect_time, send_recv_time);
    
    ofs.close();
#endif

  }
  std::cout << "(" << rank << ") finished" << std::endl;
  MPI_Finalize();

  return 0;
}
