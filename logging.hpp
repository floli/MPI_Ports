#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/attributes/mutable_constant.hpp>

#define DEBUG BOOST_LOG_TRIVIAL(debug)
#define INFO  BOOST_LOG_TRIVIAL(info)
#define ERROR  BOOST_LOG_TRIVIAL(error)

namespace logging {

using namespace boost::log;

void init(bool debug)
{
  add_common_attributes();
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  core::get()->add_global_attribute("Rank", attributes::mutable_constant<int>(0));
  attribute_cast<attributes::mutable_constant<int>>(core::get()->get_global_attributes()["Rank"]).set(rank);
  
  add_console_log(
    std::cout,
    keywords::format = "[%Rank%] [%TimeStamp%]: %Message%"
    );

  // trivial::severity_level level;
  trivial::severity_level level = debug ? trivial::debug : trivial::info;
  core::get()->set_filter(
    trivial::severity >= level
    );

  
  
}
}
