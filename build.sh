#!/bin/bash

set -e  # Exit immediately if a command fails
set -o pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

CLEAN_BUILD=false

# Parse arguments
for arg in "$@"
do
    if [[ "$arg" == "--clean" ]]; then
        CLEAN_BUILD=true
    fi
done

# Check if conan is installed
if ! command -v conan &> /dev/null
then
    echo ">>> Conan not found. Installing Conan via pip..."
    pip install --user conan
    export PATH="$HOME/.local/bin:$PATH"  # Add user pip bin to path if necessary
    echo ">>> Conan installed successfully."
else
    echo ">>> Conan is already installed."
fi

# Check if Conan default profile exists
if [ ! -f "$HOME/.conan2/profiles/default" ]; then
    echo ">>> Conan default profile not found. Running 'conan profile detect'..."
    conan profile detect
fi

if [ "$CLEAN_BUILD" = true ]; then
    echo ">>> Cleaning old build directory..."
    rm -rf "$BUILD_DIR"
fi

# Install dependencies with Conan
echo ">>> Installing dependencies via Conan..."
conan install . --output-folder="$BUILD_DIR" --build=missing

# Configure with CMake
echo ">>> Configuring project with CMake..."
cmake -B "$BUILD_DIR" -DCMAKE_TOOLCHAIN_FILE="$BUILD_DIR/conan_toolchain.cmake" -DCMAKE_BUILD_TYPE=Release

# Build the project
echo ">>> Building project..."
cmake --build "$BUILD_DIR" -j

echo ">>> Build complete!"
echo "Executable located at: $BUILD_DIR/load_balancer"
