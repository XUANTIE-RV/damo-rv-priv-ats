#!/usr/bin/env bash
# Install the Sail RISC-V formal model (pre-built binary release).
# Usage: install-sail.sh <install-dir>
# Update SAIL_VERSION to track a newer release; the CI cache key is derived
# from this script's content hash, so any edit auto-invalidates it.
# 0.14 is the first release supporting the H hypervisor extension (and several
# Sh* extensions), so it can now run this framework's hypervisor suite. The
# model config stays compatible with 0.13 and the binary is still sail_riscv_sim.

set -euo pipefail

INSTALL_DIR="${1:?Usage: install-sail.sh <install-dir>}"
SAIL_VERSION="0.14"

mkdir -p "$INSTALL_DIR"
curl --fail --location --retry 5 --retry-all-errors --retry-delay 5 \
    "https://github.com/riscv/sail-riscv/releases/download/${SAIL_VERSION}/sail-riscv-Linux-x86_64.tar.gz" \
    | tar xvz --directory="$INSTALL_DIR" --strip-components=1
