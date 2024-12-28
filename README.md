# crypto_plusplus
High-performance, low-latency live Crypto processing software written in C++

## Uses the following libraries

[uWebSockets - Web server, TBA](https://github.com/uNetworking/uWebSockets)

[libwebsockets](https://github.com/warmcat/libwebsockets)

[HTTPS Library](https://github.com/libcpr/cpr)

## Build and run - Debug version
### Using CMake directly
Specify source files (-S) as CWD "." and the build output dir (-B) as "debug"
```bash
    cmake -S . -B debug
```
Compile the build files
```bash
    cmake --build debug
```

</br>
</br>

---
### Using CMake Presets
Specify the preset configuration (debug)
```bash
    cmake --preset debug
```
Build the project using the "debug" preset
```bash
    cmake --build --preset debug
```
Run the executable
```bash
    debug/CryptoPlusPlus
```

</br>
</br>

---
### Using bash script
Handles generating build files, compiling, and running the executable 

_(If encounter permissions issue, try giving the bash file executable permission with ```chmod +x ./run_debug.sh```)_
```bash
    ./run_debug.sh
```

To run the executable direcly (without rebuilding): ```debug/CryptoPlusPlus```

</br>
</br>

# Preset Information
Debug preset sets certain compiler flags that make debugging easier (hopefully).

__CMAKE_CXX_FLAGS_DEBUG:__

__-g__: Generates debug symbols

__-fsanitize=address__: Enables AddressSanitizer (for debugging memory issues)

__-DDEBUG__: Defines the DEBUG macro for enabling assertions or other debug-only code

__CMAKE_VERBOSE_MAKEFILE__: Outputs detailed information during the build process


</br>
</br>

# Application Structure - TBA WIP

1. Launch an instance of uWebSocket on one thread, which will handle connecting to the external WebSocket server

2. Launch my application on a separate thread

3. The uWebSocket instance receives data from the external server, and stores that data in some data structure (e.g. a Queue or simple array/vector)

4. My application reads from that data structure, and processes the data 