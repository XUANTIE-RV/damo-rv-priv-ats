# =====================================================================
# ZPM (RISC-V Pointer Masking) Compliance Test - Smnpm
#
# Tests for the Smnpm extension (S-mode pointer masking via menvcfg.PMM):
#   - Group 1.b: Smnpm capability detection
#   - Group 2b:  menvcfg.PMM CSR control
#   - Group 5:   S-mode VA ignore transform
#   - Group 7.b: PM non-application - S-mode
#   - Group 8.a: MPRV - S-mode PM
#   - Group 9.a: MXR - S-mode
#   - Group 10.c: Trap - S-mode
# See DOCS/testplan/zpm_test_plan.md Smnpm-related groups.
# =====================================================================

TARGET = zpm_smnpm_test.elf
ENABLE_VM  = 1
ENABLE_PMP = 1
ENABLE_PM  = 1

# QEMU virt: explicitly request RV64 + max CPU for PM support.
QEMU_CPU   = max

# Spike: request pointer-masking extensions + Smstateen.
SPIKE_ISA_EXT = _ssnpm_smnpm_smmpm_smstateen

EXT_OBJS = \
    main.o \
    tests/test_cap.o \
    tests/test_csr.o \
    tests/test_sva.o \
    tests/test_neg.o \
    tests/test_mprv.o \
    tests/test_mxr.o \
    tests/test_trap.o

include ../common/Makefile.common
