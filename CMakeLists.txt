cmake_minimum_required (VERSION 3.5)
project(MPI_Ports LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
message(STATUS "Build configuration: " ${CMAKE_BUILD_TYPE})


add_subdirectory(EventTimings)

find_package (Threads REQUIRED)

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
set(COMPILE_FLAGS  ${COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
set(LINK_FLAGS ${LINK_FLAGS} ${MPI_LINK_FLAGS})

find_package(Boost 1.60.0
  REQUIRED
  COMPONENTS log log_setup program_options filesystem system thread unit_test_framework)
add_definitions(-DBOOST_ALL_DYN_LINK)

# add_library(EventTimings EventTimings.cpp TableWriter.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(mpiports mpiports.cpp)
target_link_libraries(mpiports ${MPI_LIBRARIES})
target_link_libraries(mpiports ${Boost_LIBRARIES})
target_link_libraries(mpiports Threads::Threads)
target_link_libraries(mpiports EventTimings)

add_executable(tests tests.cpp)
target_link_libraries(tests ${MPI_LIBRARIES})
target_link_libraries(tests ${Boost_LIBRARIES})
target_link_libraries(tests Threads::Threads)


add_executable(pub_performance publisher_performance_test.cpp)
target_link_libraries(pub_performance ${MPI_LIBRARIES})
target_link_libraries(pub_performance ${Boost_LIBRARIES})


add_executable(send_buffer send_buffer.cpp)
target_link_libraries(send_buffer ${MPI_LIBRARIES})
target_link_libraries(send_buffer ${Boost_LIBRARIES})
target_link_libraries(send_buffer Threads::Threads)

add_executable(ports_send_buffer ports_send_buffer.cpp)
target_link_libraries(ports_send_buffer ${MPI_LIBRARIES})
target_link_libraries(ports_send_buffer ${Boost_LIBRARIES})
target_link_libraries(ports_send_buffer Threads::Threads)

add_executable(ports_send_managed ports_send_managed.cpp)
target_link_libraries(ports_send_managed ${MPI_LIBRARIES})
target_link_libraries(ports_send_managed ${Boost_LIBRARIES})
target_link_libraries(ports_send_managed Threads::Threads)
