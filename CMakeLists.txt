cmake_minimum_required(VERSION 2.8)

set(Boost_INCLUDE_DIR /usr/local/include)
set(Boost_LIBRARY_DIR /usr/local/lib)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64 -pipe -O3 -g -Wall -W -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -m64 -g -Wl,-O3")
find_package(Boost COMPONENTS system program_options thread filesystem regex REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

add_executable(sqlproxy sqlproxy.cpp)
target_link_libraries(sqlproxy ${Boost_LIBRARIES})
