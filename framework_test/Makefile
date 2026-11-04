# =====================================================================
# Framework Privilege Switching Test
#
# Runs under the T-SBI S-mode-normal model: common/entry.S drops to
# S-mode before main(), and every raise back to M goes through a T-SBI
# GOTO request.  Do NOT add -DTSBI_BOOT_MODE_M here - that flag keeps the
# legacy M-mode entry for suites that own the early M environment, and
# this suite's subject under test IS the S-mode switching chain itself.
# =====================================================================

TARGET = framework_test.elf

ENABLE_VM  = 1
ENABLE_HYP = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU   = max

# Spike needs no SPIKE_ISA override: the shared string in
# common/Makefile.common already carries rv64 + H.

EXT_OBJS = \
    main.o \
    tests/test_fw_down.o \
    tests/test_fw_up.o \
    tests/test_fw_noop.o \
    tests/test_fw_roundtrip.o \
    tests/test_fw_run.o \
    tests/test_fw_csr.o \
    tests/test_fw_virt_down.o \
    tests/test_fw_virt_up.o \
    tests/test_fw_virt_indirect.o \
    tests/test_fw_run_virt.o

include ../common/Makefile.common
