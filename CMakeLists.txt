cmake_minimum_required(VERSION 3.30)
project(ThreadPool)

set(CMAKE_CXX_STANDARD 23)

add_executable(ThreadPool main.cpp
        DynamicThreadPool.h
        Task.h)
