# =====================================================================
# Hypervisor x Zawrs Cross Verification Test
# =====================================================================
#
# Tests for the Zawrs (Wait-on-Reservation-Set) extension in
# Hypervisor scenarios, matching
# DOCS/testplan/Hypervisor_Za_test_plan.md Group 6:
#   - 6.1 normal execution of wrs.nto/wrs.sto in HS/VS/VU modes
#     without spurious virtual-instruction (HZWRS-01~03)
#   - 6.2 hstatus.VTW gating of wrs.nto in VS/VU-mode:
#     virtual-instruction vs legal completion, trap report
#     (HZWRS-04~07)
#   - 6.3 mstatus.TW priority over VTW (illegal-instruction),
#     VTW scope limited to wrs.nto, VTW not affecting HS-mode
#     (HZWRS-08~12)
#
# Non-hypervisor Zawrs behavior is covered by the Zawrs/ suite
# (DOCS/testplan/Zawrs_test_plan.md).
#
# The wrs encodings are injected as raw .word instructions
# (wrs.nto = 0x00D00073, wrs.sto = 0x01D00073). Spike needs both the
# H extension and _zawrs in its ISA string.
# =====================================================================

TARGET = hypervisor_zawrs_test.elf

# Enable Hypervisor (H extension) support in the common build system.
# ENABLE_VM is required because the hyp framework (gstage_pt etc.)
# depends on the VM page-table module.
ENABLE_HYP = 1
ENABLE_VM = 1

# QEMU virt: explicitly request RV64 + H extension support (the
# "max" CPU model includes Zawrs).
QEMU_CPU = max

# Spike: explicitly request RV64 + H extension + Zawrs.
# Zama16b is appended to match the spike-rv64-max platform declaration for
# consistency across the Hypervisor_Za* suites; Zawrs has no AMO, so it does
# not change any Zawrs behavior.
SPIKE_ISA = rv64imach_zicsr_zifencei_zawrs_zama16b

# Extension-specific object files
EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
