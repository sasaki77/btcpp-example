cmake_minimum_required(VERSION 3.23)
project(btproj LANGUAGES CXX)
message(STATUS "C++ compiler ID: ${CMAKE_CXX_COMPILER_ID}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")

find_package(BehaviorTree REQUIRED)

include_directories("$ENV{EPICS_BASE}/include" "$ENV{EPICS_BASE}/include/os/Linux")
link_directories("$ENV{EPICS_BASE}/lib/linux-x86_64")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  include_directories("$ENV{EPICS_BASE}/include/compiler/gcc")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  include_directories("$ENV{EPICS_BASE}/include/compiler/clang")
else()
  message(FATAL_ERROR "Unsupported compiler for EPICS headers: ${CMAKE_CXX_COMPILER_ID}")
endif()

add_executable(btproj
  src/main.cpp
  src/mynodes.cpp
  src/mynodes.h
  src/canode.cpp
  src/canode.h
)

target_link_libraries(btproj PRIVATE BT::behaviortree_cpp ca Com)
