# Stockfish API (WIP)

## Prerequisites

Before building this project, you need to install the following dependencies:

### Required Tools
- **GCC** (GNU Compiler Collection)
- **Make** (Build automation tool)

### Required Libraries
- **libcurl** (for HTTP/HTTPS requests)

## Installation Instructions

### Ubuntu/Debian

```bash
# Update package list
sudo apt update

# Install build essentials (includes gcc and make)
sudo apt install build-essential

# Install libcurl development library
sudo apt install libcurl4-openssl-dev
```

### Fedora/RHEL/CentOS

```bash
# Install development tools (includes gcc and make)
sudo dnf groupinstall "Development Tools"

# Install libcurl development library
sudo dnf install libcurl-devel
```

### Arch Linux

```bash
# Install base development tools
sudo pacman -S base-devel

# Install curl (includes development headers)
sudo pacman -S curl
```

### Verifying Installation

After installation, verify the tools are available:

```bash
# Check gcc
gcc --version

# Check make
make --version

# Check libcurl
curl-config --version
```

## Building the Project

### Clone the repository

```bash
git clone <repository-url>
cd stockfish-api
```

### Build

```bash
# Build the project
make

# The compiled binary will be located at:
# build/stockfish-api
```

### Clean build artifacts

```bash
# Remove all compiled files
make clean
```

## Running the Project

```bash
# Run the compiled binary
./build/stockfish-api

# Run with arguments
./build/stockfish-api arg1 arg2 arg3
```
