#!/usr/bin/env bash
# Install Spike RISC-V ISA Simulator from source.
# Usage: install-spike.sh <install-dir>
# Update SPIKE_COMMIT to track a newer revision; the CI cache key is derived
# from this script's content hash, so any edit here invalidates the cache.

set -euo pipefail

INSTALL_DIR="${1:?Usage: install-spike.sh <install-dir>}"
SPIKE_COMMIT="3d8eb089bd289c59dcb506f197a172e02beb7b5b"

# ---- Build dependencies ----------------------------------------------------
# libboost-system-dev: satisfies the AX_BOOST_ASIO autoconf macro which
# probes for libboost_system even though ASIO is header-only since Boost 1.69.
# See: https://github.com/riscv-software-src/riscv-isa-sim/issues/1289
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    device-tree-compiler \
    libboost-dev \
    libboost-system-dev \
    libboost-regex-dev

# ---- Build and install ------------------------------------------------------
git clone https://github.com/riscv/riscv-isa-sim.git /tmp/riscv-isa-sim
cd /tmp/riscv-isa-sim
git checkout "$SPIKE_COMMIT"
mkdir build && cd build
../configure --prefix="$INSTALL_DIR"
make -j"$(nproc)"
make install
