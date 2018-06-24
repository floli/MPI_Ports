cmake_minimum_required (VERSION 3.5)
project(MPI_Ports LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

add_subdirectory(EventTimings)

find_package (Threads REQUIRED)

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
set(COMPILE_FLAGS  ${COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
set(LINK_FLAGS ${LINK_FLAGS} ${MPI_LINK_FLAGS})

find_package(Boost 1.60.0
  REQUIRED
  COMPONENTS log log_setup program_options filesystem system thread)
add_definitions(-DBOOST_ALL_DYN_LINK)

# add_library(EventTimings EventTimings.cpp TableWriter.cpp)


add_executable(mpiports mpiports.cpp)
target_link_libraries(mpiports ${MPI_LIBRARIES})
target_link_libraries(mpiports ${Boost_LIBRARIES})
target_link_libraries(mpiports Threads::Threads)
target_link_libraries(mpiports EventTimings)

add_executable(pub_performance publisher_performance_test.cpp)
target_link_libraries(pub_performance ${MPI_LIBRARIES})
target_link_libraries(pub_performance ${Boost_LIBRARIES})


add_executable(mpi_buffer mpi_buffer.cpp)
target_link_libraries(mpi_buffer ${MPI_LIBRARIES})
target_link_libraries(mpi_buffer ${Boost_LIBRARIES})
target_link_libraries(mpi_buffer Threads::Threads)
