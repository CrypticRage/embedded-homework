cmake_minimum_required(VERSION 3.12)

# set the project name
project(count)

# add the executable
add_executable(count
    count.cpp
    defines.hpp
    command.cpp
    command.hpp
    socket.hpp
)
