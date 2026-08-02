# =====================================================================
# Ssqosid Extension Verification Test
# =====================================================================
#
# Tests for the Ssqosid extension (QoS Identifiers).
# See DOCS/testplan/Ssqosid_test_plan.md for the test plan.
#
# Note: Hypervisor-dependent tests (V=1, SRMCFG-19~24) have been
# moved to Hypervisor_Ssqosid/.
# =====================================================================

TARGET = ssqosid_test.elf

# Spike ISA extensions
SPIKE_ISA_EXT = _ssqosid_smstateen

# Extension-specific object files
EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
