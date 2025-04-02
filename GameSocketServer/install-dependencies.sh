#!/bin/bash
# install-dependencies.sh
# Script to install all required dependencies for the project on Ubuntu

# Color codes for echo
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${CYAN}===== Installing dependencies for the project =====${NC}"

# Update package lists
echo -e "${YELLOW}Updating package lists...${NC}"
sudo apt-get update

# Install build essentials and other required packages
echo -e "${YELLOW}Installing build tools and libraries...${NC}"
sudo apt-get install -y build-essential cmake git curl zip unzip tar pkg-config

# Install required libraries
echo -e "${YELLOW}Installing library dependencies...${NC}"
sudo apt-get install -y libssl-dev libpq-dev libboost-all-dev

# Install vcpkg if not already installed
if [ ! -d "./vcpkg" ]; then
    echo -e "${YELLOW}Installing vcpkg...${NC}"
    git clone https://github.com/Microsoft/vcpkg.git
    ./vcpkg/bootstrap-vcpkg.sh -disableMetrics
else
    echo -e "${YELLOW}vcpkg already installed. Updating...${NC}"
    cd vcpkg
    git pull
    cd ..
fi

# Install required packages with vcpkg
echo -e "${YELLOW}Installing required packages with vcpkg...${NC}"
./vcpkg/vcpkg install boost-system boost-asio boost-beast libpqxx nlohmann-json spdlog fmt openssl --triplet=x64-linux

# Create a vcpkg.cmake file for easy integration with CMake
echo -e "${YELLOW}Creating vcpkg.cmake for CMake integration...${NC}"
echo "set(CMAKE_TOOLCHAIN_FILE \"${PWD}/vcpkg/scripts/buildsystems/vcpkg.cmake\" CACHE STRING \"\")" > vcpkg.cmake

echo -e "${GREEN}Dependencies installation completed!${NC}"
echo -e "${YELLOW}To build the project, run: ./rebuild.sh${NC}"