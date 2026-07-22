# =====================================================================
# ZPM (RISC-V Pointer Masking) Compliance Test - Ssnpm
#
# Tests for the Ssnpm extension (U-mode pointer masking via senvcfg.PMM):
#   - Group 1.a: Ssnpm capability detection
#   - Group 2a:  senvcfg.PMM CSR control
#   - Group 3:   U-mode VA ignore transform (load/store)
#   - Group 4:   U-mode VA ignore transform (AMO)
#   - Group 7.a: PM non-application - U-mode
#   - Group 8.b: MPRV - U-mode PM
#   - Group 10.a: Trap - U-mode stval
# See DOCS/testplan/zpm_test_plan.md Ssnpm-related groups.
# =====================================================================

TARGET = zpm_ssnpm_test.elf
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
    tests/test_uva.o \
    tests/test_uamo.o \
    tests/test_neg.o \
    tests/test_mprv.o \
    tests/test_trap.o

include ../common/Makefile.common
