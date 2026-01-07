cmake_minimum_required(VERSION 3.23)
project(btproj LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")

find_package(BehaviorTree REQUIRED)

add_executable(btproj
  src/main.cpp
  src/mynodes.cpp
  src/mynodes.h
)

target_link_libraries(btproj PRIVATE BT::behaviortree_cpp)
