# Builds and runs the debug version of the application
#!/bin/bash

#Variables
EXECUTABLE="./debug/CryptoPlusPlus"

#Run CMake and generate build files - use debug preset
echo "Running CMake - generating build files - using 'debug'"
cmake --preset debug

#Compile build files
echo "Compiling build files"
cmake --build --preset debug

# Check if previous compile command executed successfully
# '$?' = exit status of last command, '-ne 0'= "not equal to zero"
if [ $? -ne 0 ]; then
    echo "cmake build failed. Exiting."
    exit 1
fi

echo "Build succeeded, running executable"
$EXECUTABLE