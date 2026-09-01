# =====================================================================
# Hypervisor x Ssccfg Cross Test
#
# Tests for Smcdeleg/Ssccfg counter-delegation behavior in Hypervisor
# scenarios (Hypervisor_Ss_test_plan.md Group 11, HCROSS-SSCCFG-01~24):
#   - Group 11.1: scountovf virtualization (VS/VU read -> virtual-inst)
#   - Group 11.2: scountinhibit VS/VU access -> virtual-inst
#   - Group 11.3: hvip/hvien LCOFI bit (bit 13) writability
#   - Group 11.4: vsiselect/vsireg* multi-privilege access rules
#   - Group 11.5: hstateen0 bit 60 gating of VS-mode access
# =====================================================================

TARGET     = hypervisor_ssccfg_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU   = max

# Spike: request RV64 + H extension + indirect CSR access + counter
# delegation + Sscofpmf (needed by the scountovf virtualization
# sub-group).
SPIKE_ISA_EXT = _sscsrind_ssccfg_sscofpmf

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
