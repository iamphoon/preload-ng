#!/bin/bash
# Installation script for preload-ng
# This script compiles and optionally installs preload

set -e

echo "=========================================="
echo "  Preload-NG - Installation Script"
echo "=========================================="
echo ""

# Check dependencies
echo "[1/4] Checking dependencies..."

check_command() {
    if ! command -v "$1" &>/dev/null; then
        echo "Error: '$1' not found. Please install it first."
        exit 1
    fi
}

check_command automake
check_command autoconf
check_command libtool
check_command make
check_command gcc

# Move to source directory
if [ -d "preload-src" ]; then
    cd preload-src
elif [ -d "../preload-src" ]; then
    cd ../preload-src
else
    echo "Error: preload-src directory not found."
    exit 1
fi

# Clean previous build if Makefile exists
if [ -f Makefile ]; then
    echo "Cleaning previous build..."
    make distclean
fi

# Build system generation
echo "[2/4] Configuring build system..."
./bootstrap
./configure
echo "[3/4] Compiling..."
make

echo "=========================================="
echo "  Build completed successfully!"
echo "=========================================="

# Ask if user wants to install
read -p "Do you want to install preload? (y/N): " response

case "$response" in
[yY] | [yY][eE][sS])
    echo ""
    echo "Installing preload (requires root permissions)..."
    sudo make install
    sudo systemctl enable --now preload
    echo ""
    echo "✓ Preload installed successfully!"
    echo ""
    ;;
*)
    echo ""
    echo "Installation cancelled."
    echo "To install manually, run: sudo make install"
    ;;
esac
