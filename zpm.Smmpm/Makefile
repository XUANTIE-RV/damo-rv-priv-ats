# =====================================================================
# ZPM (RISC-V Pointer Masking) Compliance Test - Smmpm
#
# Tests for the Smmpm extension (M-mode pointer masking via mseccfg.PMM):
#   - Group 1.c: Smmpm capability detection
#   - Group 2c:  mseccfg.PMM CSR control
#   - Group 6:   M-mode PA ignore transform
#   - Group 8.c: MPRV - M-mode PM
#   - Group 9.b: MXR - M-mode
#   - Group 10.b: Trap - M-mode mtval
# See DOCS/testplan/zpm_test_plan.md Smmpm-related groups.
# =====================================================================

TARGET = zpm_smmpm_test.elf
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
    tests/test_mpa.o \
    tests/test_mprv.o \
    tests/test_mxr.o \
    tests/test_trap.o

include ../common/Makefile.common
