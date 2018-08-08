#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include "common.hpp"
#include "prettyprint.hpp"
#include <iostream>
using namespace std;
using namespace mp;


BOOST_AUTO_TEST_CASE(testGetRanksAbsolute)
{
  BOOST_TEST( getRanks(2, 8, 3) == std::vector<int>({2, 3}) );
  BOOST_TEST( getRanks(3, 8, 3) == std::vector<int>({2, 3, 4}) );
  BOOST_TEST( getRanks(4, 8, 3) == std::vector<int>({1, 2, 3, 4}) );
  BOOST_TEST( getRanks(5, 8, 3) == std::vector<int>({1, 2, 3, 4, 5}) );
  BOOST_TEST( getRanks(6, 8, 3) == std::vector<int>({0, 1, 2, 3, 4, 5}) );
  BOOST_TEST( getRanks(7, 8, 3) == std::vector<int>({0, 1, 2, 3, 4, 5, 6}) );
  BOOST_TEST( getRanks(8, 8, 3) == std::vector<int>({0, 1, 2, 3, 4, 5, 6, 7}) );

  BOOST_TEST( getRanks(8, 80, 3) == std::vector<int>({0, 1, 2, 3, 4, 5, 6, 7}) );
  
  BOOST_TEST( getRanks(3, 80, 0) == std::vector<int>({0, 1, 2}) );

  BOOST_TEST( getRanks(4, 5, 4) == std::vector<int>({1, 2, 3, 4}) );

  BOOST_TEST( getRanks(4, 5, 4) == std::vector<int>({1, 2, 3, 4}) );
 
}

BOOST_AUTO_TEST_CASE(testGetRanksRatio)
{
  BOOST_TEST( getRanks(0.2, 10, 3) == std::vector<int>({2, 3}) );
  BOOST_TEST( getRanks(0.5, 10, 3) == std::vector<int>({1, 2, 3, 4, 5}) );
  BOOST_TEST( getRanks(0.8, 10, 3) == std::vector<int>({0, 1, 2, 3, 4, 5, 6, 7}) );
}

BOOST_AUTO_TEST_CASE(testInvertGetRanksAbsolute)
{
  BOOST_TEST( invertGetRanks(2, 8, 3) == std::vector<int>({3, 4}) );
  BOOST_TEST( invertGetRanks(3, 8, 3) == std::vector<int>({2, 3, 4}) );
  BOOST_TEST( invertGetRanks(4, 8, 3) == std::vector<int>({0, 1, 2, 3, 4, 5}) );
  BOOST_TEST( invertGetRanks(5, 8, 3) == std::vector<int>({0, 1, 2, 3, 4, 5, 6, 7}) );
  BOOST_TEST( invertGetRanks(6, 8, 3) == std::vector<int>({0, 1, 2, 3, 4, 5, 6, 7}) );
  BOOST_TEST( invertGetRanks(7, 8, 3) == std::vector<int>({0, 1, 2, 3, 4, 5, 6, 7}) );
  BOOST_TEST( invertGetRanks(8, 8, 3) == std::vector<int>({0, 1, 2, 3, 4, 5, 6, 7}) );

  BOOST_TEST( invertGetRanks(8, 80, 3) == std::vector<int>({0, 1, 2, 3, 4, 5, 6, 7}) );
  
  BOOST_TEST( invertGetRanks(3, 80, 0) == std::vector<int>({0, 1}) );

  BOOST_TEST( invertGetRanks(4, 5, 4) == std::vector<int>({3, 4}) );

  BOOST_TEST( invertGetRanks(4, 5, 4) == std::vector<int>({3, 4}) );
 
}
