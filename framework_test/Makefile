# =====================================================================
# Framework Privilege Switching Test
# =====================================================================

TARGET = framework_test.elf

ENABLE_VM  = 1
ENABLE_HYP = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU   = max

# Spike: explicitly request RV64 + H extension + Sscsrind.
SPIKE_ISA = rv64imach_zicsr_zifencei


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
