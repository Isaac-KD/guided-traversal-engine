cmake_minimum_required(VERSION 3.10)
project(GuidedTraversalEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra")

include_directories(src)

add_executable(guided_traversal_engine 
    src/main.cpp 
    src/IndexReader.cpp 
    src/PostingIterator.cpp 
    src/MaxScoreGT.cpp
    src/Metrics.cpp
)
