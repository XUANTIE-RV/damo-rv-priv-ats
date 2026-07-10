# =====================================================================
# Sha (Augmented Hypervisor) Extension Compliance Test
#
# Tests for the Sha composite extension. See
# DOCS/testplan/sha_test_plan.md for the plan.
#
# Sha = H + Ssstateen + Shcounterenw + Shvstvala + Shtvala +
#        Shvstvecd + Shvsatpa + Shgatpa
# =====================================================================

TARGET     = sha_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1

# QEMU virt: RV64 + H extension + all translation modes.
QEMU_CPU   = max

# Spike: explicitly request RV64 + H extension support.
# smcsrind_sscsrind needed for SHA-HCTL-05 (hstateen0 CSRIND gate test).
SPIKE_ISA = rv64imach_zicsr_zifencei_smstateen_zicntr_smcsrind_sscsrind

EXT_OBJS = \
    main.o \
    tests/sha_strap.o \
    tests/test_register.o

include ../common/Makefile.common
