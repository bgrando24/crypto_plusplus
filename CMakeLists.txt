# Min CMake version
cmake_minimum_required(VERSION 3.16)

# Project name and version
project(CryptoPlusPlus VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add executable target - name of executable, and the source files to compile
add_executable(CryptoPlusPlus src/main.cpp)

#link external libraries

# The cpr library: https://github.com/libcpr/cpr
include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG dec9422db3af470641f8b0d90e4b451c4daebf64) # Replace with your desired git commit from: https://github.com/libcpr/cpr/releases
FetchContent_MakeAvailable(cpr)

target_link_libraries(CryptoPlusPlus PRIVATE cpr::cpr)