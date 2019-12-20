cmake_minimum_required(VERSION 3.14)
project(Osp3 C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_FLAGS "-fprofile-arcs -ftest-coverage -lgcov")

#set(CFLAGS="")
#set(LDFLAGS="-lgcov")

add_executable(Osp3 main.c ConcurrentQueue.c TMessage.h)