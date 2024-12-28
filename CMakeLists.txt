# Min CMake version
cmake_minimum_required(VERSION 3.16)

# Project name and version
project(CryptoPlusPlus VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable debugging
add_definitions(-DLWS_LOG_LEVEL=DEBUG)

# Add executable target - name of executable, and the source files to compile
add_executable(CryptoPlusPlus src/main.cpp src/websocket_client.cpp)

#link external libraries

# The cpr library: https://github.com/libcpr/cpr
include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG dec9422db3af470641f8b0d90e4b451c4daebf64) # Replace with your desired git commit from: https://github.com/libcpr/cpr/releases
FetchContent_MakeAvailable(cpr)

# ----- TBA ----- Add uWebSocket repo to project: https://github.com/uNetworking/uWebSockets
# FetchContent_Declare(uWebSockets GIT_REPOSITORY https://github.com/uNetworking/uWebSockets
#                                  GIT_TAG b9b59b2b164489f3788223fec5821f77f7962d43)  # https://github.com/uNetworking/uWebSockets/commit/b9b59b2b164489f3788223fec5821f77f7962d43
# FetchContent_MakeAvailable(uWebSockets)

# Add include directories for uWebSockets
# target_include_directories(CryptoPlusPlus PRIVATE ${uWebSockets_SOURCE_DIR})


# ----- Adding libwebsockets: https://github.com/warmcat/libwebsockets -----
# fetch repo
FetchContent_Declare(libwebsockets GIT_REPOSITORY https://github.com/warmcat/libwebsockets
                                   GIT_TAG v4.3.3)  # https://github.com/warmcat/libwebsockets/releases/tag/v4.3.3
# populate dependency
FetchContent_MakeAvailable(libwebsockets)

# link libwebsocket dependencies
find_package(OPENSSL REQUIRED)
find_package(ZLIB REQUIRED)

# Link external libraries
target_link_libraries(CryptoPlusPlus PRIVATE cpr::cpr websockets OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB)