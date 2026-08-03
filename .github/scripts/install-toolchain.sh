#!/usr/bin/env bash
# Install a pre-built RISC-V cross toolchain (GCC or Clang/LLVM) from
# riscv-collab/riscv-gnu-toolchain releases.
# Usage: install-toolchain.sh <gcc|clang> <install-dir>
# Update TOOLCHAIN_VERSION to track a newer release; the CI cache key is
# derived from this script's content hash, so any edit auto-invalidates it.

set -euo pipefail

TOOL="${1:?Usage: install-toolchain.sh <gcc|clang> <install-dir>}"
INSTALL_DIR="${2:?Usage: install-toolchain.sh <gcc|clang> <install-dir>}"
TOOLCHAIN_VERSION="2026.06.06"

# Map toolchain name to release asset suffix:
#   gcc   -> riscv64-elf-ubuntu-22.04-gcc.tar.xz
#   clang -> riscv64-elf-ubuntu-22.04-llvm.tar.xz
if [ "$TOOL" = "clang" ]; then
    ASSET_SUFFIX="llvm"
else
    ASSET_SUFFIX="$TOOL"
fi

# ---- Dependencies -----------------------------------------------------------
sudo apt-get update
sudo apt-get install -y xz-utils

# ---- Download and extract ---------------------------------------------------
mkdir -p "$INSTALL_DIR"
curl --fail --location --retry 5 --retry-all-errors --retry-delay 5 \
    "https://github.com/riscv-collab/riscv-gnu-toolchain/releases/download/${TOOLCHAIN_VERSION}/riscv64-elf-ubuntu-22.04-${ASSET_SUFFIX}.tar.xz" \
    | tar xvJ --directory="$INSTALL_DIR" --strip-components=1
