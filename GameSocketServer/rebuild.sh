#!/bin/bash
# rebuild.sh
# Script to build the project on Ubuntu

# Color codes for echo
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Check and save current location
CURRENT_DIR=$(pwd)

# Verify we are in the project root directory
if [ ! -f "./CMakeLists.txt" ]; then
    echo -e "${RED}Error: This script must be run from the project root directory."
    echo -e "Please run the script from the directory containing CMakeLists.txt.${NC}"
    exit 1
fi

echo -e "${CYAN}===== Starting Build Script =====${NC}"

# Check if vcpkg is installed
if [ ! -d "./vcpkg" ]; then
    echo -e "${YELLOW}vcpkg not found. Installing...${NC}"
    git clone https://github.com/Microsoft/vcpkg.git
    ./vcpkg/bootstrap-vcpkg.sh -disableMetrics
    
    echo -e "${YELLOW}Installing required packages with vcpkg...${NC}"
    ./vcpkg/vcpkg install boost-system boost-asio boost-beast libpqxx nlohmann-json spdlog fmt openssl --triplet=x64-linux
else
    echo -e "${GREEN}vcpkg already installed.${NC}"
fi

# Check for required system packages
echo -e "${YELLOW}Checking for required system packages...${NC}"
PACKAGES="build-essential cmake git pkg-config libpq-dev libssl-dev"
INSTALL_CMD="sudo apt-get install -y"

for pkg in $PACKAGES; do
    if ! dpkg -l | grep -q $pkg; then
        echo -e "${YELLOW}Installing $pkg...${NC}"
        $INSTALL_CMD $pkg
    fi
done

# 1. Delete build directory
echo -e "${YELLOW}1. Deleting build directory...${NC}"
if [ -d "./build" ]; then
    rm -rf ./build
    echo -e "${GREEN}   Existing build directory deleted${NC}"
else
    echo -e "${YELLOW}   Build directory does not exist. Creating a new one.${NC}"
fi

# 2. Create new build directory
echo -e "${YELLOW}2. Creating new build directory...${NC}"
mkdir -p ./build
echo -e "${GREEN}   Build directory created${NC}"

# 3. Navigate to build directory
cd ./build

# 4. Generate CMake files
echo -e "${YELLOW}3. Generating CMake files...${NC}"
cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
if [ $? -ne 0 ]; then
    echo -e "${RED}   CMake generation failed${NC}"
    cd $CURRENT_DIR  # Return to original location
    exit 1
fi
echo -e "${GREEN}   CMake generation completed${NC}"

# 5. Perform build
echo -e "${YELLOW}4. Building project...${NC}"
cmake --build .
if [ $? -ne 0 ]; then
    echo -e "${RED}   Build failed${NC}"
    cd $CURRENT_DIR  # Return to original location
    exit 1
else
    echo -e "${GREEN}   Build successful${NC}"
fi

# 6. Return to original location
cd $CURRENT_DIR

echo -e "${CYAN}===== Build Script Completed =====${NC}"
echo -e "${YELLOW}Run the server with: ./build/SocketServer${NC}"