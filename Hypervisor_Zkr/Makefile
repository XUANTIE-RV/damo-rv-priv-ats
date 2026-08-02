# =====================================================================
# Hypervisor x Zkr Cross Verification Test
# =====================================================================
#
# Tests for the Zkr (Entropy Source) extension in Hypervisor scenarios:
#   - HS-mode seed CSR access control via mseccfg.SSEED (ZKR-HYP-01/02)
#   - VS/VU-mode seed CSR access: virtual-instruction vs illegal
#     exception distinction (ZKR-HYP-03~11)
#   - Read-only access exception priority (ZKR-HYP-12)
#
# See DOCS/testplan/Hypervisor_cross_test_plan.md Group 17.
#
# These tests were extracted from Zkr_test_plan.md Group 7 (VS/VU-mode)
# and the HS-mode cases from Group 6, renumbered to ZKR-HYP-01~12.
# =====================================================================

TARGET = hypervisor_zkr_test.elf

# Enable Hypervisor (H extension) support in the common build system.
# ENABLE_VM is required because the hyp framework (two_stage/gstage_pt)
# depends on the VM page-table module.
ENABLE_HYP = 1
ENABLE_VM = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU = max

# Spike: explicitly request RV64 + H extension + Zkr.
SPIKE_ISA = rv64imach_zicsr_zifencei_zkr

# Extension-specific object files
EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
