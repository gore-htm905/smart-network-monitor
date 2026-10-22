cmake_minimum_required(VERSION 3.10)
project(SmartNetworkMonitor)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(BackendServer
    src/main.cpp
    src/ApiServer.cpp
)

# Optional: Link against threading library (required by cpp-httplib on some systems)
find_package(Threads REQUIRED)
target_link_libraries(BackendServer Threads::Threads)
