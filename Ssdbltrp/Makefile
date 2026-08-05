# =====================================================================
# Ssdbltrp Extension Verification Test
# =====================================================================

TARGET = ssdbltrp_test.elf

# QEMU xt-c930v-cp CPU implements Ssdbltrp extension
QEMU_CPU   = xt-c930v-cp

SPIKE_ISA_EXT = _ssdbltrp

# The double-trap machinery interacts with Smdbltrp semantics: on
# trap entry into M-mode the hart may set mstatus.MDT, and while
# MDT=1 any further M-mode exception is an unrecoverable critical
# error (no Smrnmi on these platforms).  Enable the framework's
# clear_mdt() paths (TEST_END/TEST_SKIP/reset_state) so no test ever
# leaves MDT set.
CFLAGS += -DSMDBLTRP_SUPPORTED

EXT_OBJS = \
    ssdbltrp_trap_asm.o \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
