#pragma once

#include <mpi.h>
#include <memory>
#include <deque>
#include <mutex>
#include <boost/variant.hpp>
class Request;
using PtrRequest = std::shared_ptr<Request>;


class Request
{
public:
  MPI_Request _request;
  
  Request(MPI_Request request)
    : _request(request)
  {}

  void wait(std::vector<PtrRequest> &requests)
  {
    for (auto request : requests) {
      request->wait();
    }
  }

  bool test()
  {
    int complete = 0;
    MPI_Test(&_request, &complete, MPI_STATUS_IGNORE);
    return complete;
  }

  void wait()
  {
    MPI_Wait(&_request, MPI_STATUS_IGNORE);
  }
};

class release_visitor : public boost::static_visitor<>
{
public:
  template<typename T>
  void operator()(T & operand) const
  {
    operand.release();
  }
};

class is_valid_ptr_visitor : public boost::static_visitor<bool>
{
public:
  template<typename T>
  bool operator()(T & operand) const
  {
    return static_cast<bool>(operand);
  }
};


class BufferManager
{
public:
  /// Deleted copy operator for singleton pattern
  BufferManager(BufferManager const &) = delete;
  
  /// Deleted assigment operator for singleton pattern
  void operator=(BufferManager const &) = delete;

  static BufferManager & instance()
  {
    static BufferManager instance;
    return instance;
  }

  template<typename T>
  void put(std::unique_ptr<Request> request, std::unique_ptr<T> buffer)
  {
    std::lock_guard<std::mutex> lock(bufferedRequestsMutex);
    bufferedRequests.emplace_back(std::move(request), std::move(buffer));
  }

  void run()
  {
    INFO << "Starting thread";
    thread = std::thread(&BufferManager::check, this);
  }

  void stop()
  {
    stopFlag = true;
    thread.join();
  }

  
  void check()
  {
    INFO << "Check thread started";
    while (not stopFlag) {
      for (auto it = bufferedRequests.begin(); it != bufferedRequests.end();) {
        if (boost::apply_visitor(is_valid_ptr_visitor(), it->second) and it->first->test()) {
          std::lock_guard<std::mutex> lock(bufferedRequestsMutex);
          it->first.release();
          boost::apply_visitor(release_visitor(), it->second);
          DEBUG << "Released a ptr, #requests = " << bufferedRequests.size();
          it = bufferedRequests.erase(it);
          sleep(100);
        }
        else {
          ++it;
        }
      }
      std::this_thread::yield();
    }
  }
    

private:
  /// Private, empty constructor for singleton pattern
  BufferManager() {}

  using ptr_types = boost::variant< std::unique_ptr<int>, std::unique_ptr<double> >;

  std::list<std::pair<std::unique_ptr<Request>, ptr_types>> bufferedRequests;
  
  std::thread thread;
  std::mutex bufferedRequestsMutex;
  bool stopFlag = false;
};
