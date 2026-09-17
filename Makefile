# =====================================================================
# Top-level Makefile for RISC-V Privilege Test Framework
#
# Usage:
#   make                    # Print help
#   make <group>_group      # Build one group (mirrors run_case_stats.sh)
#                           #   groups: hyp ss sv sd sm pmp cfi zpm cmo zi zc zk za
#   make all                # Build ALL extensions
#   make pmp                # Build a single extension
#   make spike              # Run all on Spike
#   make spike-pmp          # Run one on Spike
#   make sail-pmp           # Run one on Sail
#   make qemu-pmp           # Run one on QEMU
#   make whisper-<ext>      # Run one on Whisper
#   make clean              # Clean all
# =====================================================================

# =====================================================================
# Extension groups (mirrors the suite lists in run_case_stats.sh)
# =====================================================================

# --- Hypervisor extensions ---
HYP_BASE_GROUP = Hypervisor_CSR Hypervisor_Interrupts Hypervisor_Exceptions Sha Shcounterenw Shgatpa Shlcofideleg Shtvala Shvstvala Shvsatpa
HYP_VM_GROUP   = Sv39x4 Sv48x4 Sv57x4 \
                 Sv39x4_Sv39 Sv39x4_Sv48 Sv39x4_Sv57 \
                 Sv48x4_Sv39 Sv48x4_Sv48 Sv48x4_Sv57 \
                 Sv57x4_Sv39 Sv57x4_Sv48 Sv57x4_Sv57
HYP_SM_GROUP   = Hypervisor_Smcntrpmf Hypervisor_Smcsrind Hypervisor_Smmpm Hypervisor_Smnpm Hypervisor_Smstateen Hypervisor_PMP
HYP_SS_GROUP   = Hypervisor_Ssccfg Hypervisor_Ssccptr Hypervisor_Sscofpmf Hypervisor_Sscsrind Hypervisor_Ssdbltrp Hypervisor_Ssnpm Hypervisor_Ssqosid Hypervisor_Ssstateen Hypervisor_Sstc Hypervisor_Sstvala
HYP_SV_GROUP   = Hypervisor_Svadu Hypervisor_Svinval Hypervisor_Svnapot Hypervisor_Svpbmt
HYP_ZI_GROUP   = Hypervisor_Zicbom Hypervisor_Zicbop Hypervisor_Zicboz Hypervisor_Zicfilp Hypervisor_Zicfiss Hypervisor_Zkr Hypervisor_Zihintntl Hypervisor_Zicntr Hypervisor_Zihpm Hypervisor_Vector
HYP_ZA_GROUP   = Hypervisor_Zalrsc Hypervisor_Zaamo Hypervisor_Zacas Hypervisor_Zabha Hypervisor_Zalasr Hypervisor_Zawrs
HYP_ZC_GROUP   = Hypervisor_Zca

# Union of all Hypervisor groups
HYP_GROUP = $(HYP_BASE_GROUP) $(HYP_VM_GROUP) $(HYP_SM_GROUP) $(HYP_SS_GROUP) \
            $(HYP_SV_GROUP) $(HYP_ZI_GROUP) $(HYP_ZA_GROUP) $(HYP_ZC_GROUP)

# --- Supervisor extensions ---
SS_GROUP  = Ss_CSR Ss_Exceptions Ss_Interrupts Ssccfg Ssccptr Sscofpmf Sscounterenw Sscsrind Ssctr Ssdbltrp Ssstateen Sstc Sstvala Sstvecd Ssu64xl
SV_GROUP  = Sv39 Sv48 Sv57 Svbare Svade Svadu Svnapot Svinval Svpbmt Svvptc

# --- Debug extensions ---
SD_GROUP  = Sdext Sdtrig

# --- Machine extensions ---
SM_GROUP  = Sm_CSR Sm_Exceptions Sm_Interrupts Smcdeleg Smcntrpmf Smcsrind Smctr Smdbltrp Smstateen
PMP_GROUP = pmp pmp_sv39 pmp_sv48 pmp_sv57

# --- CFI extensions ---
CFI_GROUP = cfi.Zicfilp cfi.Zicfiss

# --- Pointer masking extensions ---
ZPM_GROUP = zpm.Smmpm zpm.Smnpm zpm.Ssnpm

# --- CMO extensions ---
CMO_GROUP = cmo.base cmo.Zicbom cmo.Zicbop cmo.Zicboz

# --- Zi extensions ---
ZI_GROUP  = Zicntr Zicond Zicsr Zifencei Zihintntl Zihintpause Zihpm Zimop Ziccamoa Ziccamoc Ziccid Ziccif Zicclsm Ziccrse

# --- Zc extensions ---
ZC_GROUP  = Zcmop Zcmp Zcmt

# --- Zk extensions ---
ZK_GROUP  = Zkr Zkt

# --- Za extensions ---
ZA_GROUP  = Za64rs Za128rs Zaamo Zabha Zacas Zalasr Zalrsc Zama16b Zawrs

# All extensions (union of groups, mirrors ALL_SUITES in run_case_stats.sh)
EXTENSIONS = $(HYP_GROUP) $(SS_GROUP) $(SV_GROUP) $(SD_GROUP) $(SM_GROUP) \
             $(PMP_GROUP) $(CFI_GROUP) $(ZPM_GROUP) $(CMO_GROUP) \
             $(ZI_GROUP) $(ZK_GROUP) $(ZC_GROUP) $(ZA_GROUP)

# Forward all variables to sub-makes
MAKE_VARS = $(if $(XLEN),XLEN=$(XLEN)) \
            $(if $(CONFIG),CONFIG=$(CONFIG)) \
            $(if $(CROSS_COMPILER),CROSS_COMPILER=$(CROSS_COMPILER)) \
            $(if $(TOOLCHAIN),TOOLCHAIN=$(TOOLCHAIN)) \
            $(if $(LOG_LEVEL),LOG_LEVEL=$(LOG_LEVEL)) \
            $(if $(SAIL),SAIL=$(SAIL)) \
            $(if $(SPIKE),SPIKE=$(SPIKE)) \
            $(if $(SPIKE_ISA),SPIKE_ISA=$(SPIKE_ISA)) \
            $(if $(WHISPER),WHISPER=$(WHISPER))

# Generate sail-<ext>, spike-<ext>, qemu-<ext>, and whisper-<ext> targets
SAIL_TARGETS  = $(addprefix sail-,$(EXTENSIONS))
SPIKE_TARGETS = $(addprefix spike-,$(EXTENSIONS))
QEMU_TARGETS  = $(addprefix qemu-,$(EXTENSIONS))
WHISPER_TARGETS = $(addprefix whisper-,$(EXTENSIONS))

.PHONY: help all clean sail spike qemu whisper \
        hyp_group ss_group sv_group sd_group sm_group pmp_group \
        cfi_group zpm_group cmo_group zi_group zc_group zk_group za_group \
        $(EXTENSIONS) $(SAIL_TARGETS) $(SPIKE_TARGETS) $(QEMU_TARGETS) $(WHISPER_TARGETS)

# Default target: print usage help
.DEFAULT_GOAL := help

help:
	@echo "====================================================================="
	@echo "  RISC-V Privilege Test Framework - Build System"
	@echo "====================================================================="
	@echo ""
	@echo "  Group targets:"
	@echo "    make hyp_group        Build all Hypervisor extensions"
	@echo "    make ss_group         Build Supervisor (Ss*) extensions"
	@echo "    make sv_group         Build Supervisor VM (Sv*) extensions"
	@echo "    make sd_group         Build Debug (Sd*) extensions"
	@echo "    make sm_group         Build Machine (Sm*) extensions"
	@echo "    make pmp_group        Build PMP extensions"
	@echo "    make cfi_group        Build CFI (Zicfilp/Zicfiss) extensions"
	@echo "    make zpm_group        Build pointer-masking (Zpm) extensions"
	@echo "    make cmo_group        Build CMO (Zicb*) extensions"
	@echo "    make zi_group         Build Zi* extensions"
	@echo "    make zc_group         Build Zc* extensions"
	@echo "    make zk_group         Build Zk* extensions"
	@echo "    make za_group         Build Za* extensions"
	@echo "    make all              Build ALL extensions"
	@echo ""
	@echo "  Single extension:"
	@echo "    make <ext>            Build one (e.g., make pmp, make aia, make zpm)"
	@echo ""
	@echo "  Simulator targets:"
	@echo "    make spike            Run all on Spike"
	@echo "    make spike-<ext>      Run one on Spike (e.g., make spike-pmp)"
	@echo "    make sail             Run all on Sail"
	@echo "    make sail-<ext>       Run one on Sail"
	@echo "    make qemu-<ext>       Run one on QEMU"
	@echo "    make whisper           Run all on Whisper"
	@echo "    make whisper-<ext>     Run one on Whisper (e.g., make whisper-Sv39)"
	@echo ""
	@echo "  Options:"
	@echo "    XLEN=32|64            Architecture (default: 64)"
	@echo "    CROSS_COMPILER=...    Toolchain prefix (e.g., riscv64-unknown-elf-)"
	@echo "    TOOLCHAIN=gcc|clang   Compiler backend (default: gcc)"
	@echo "    CONFIG=...            Target configuration"
	@echo "    LOG_LEVEL=1-6         Verbosity (default: 3)"
	@echo ""
	@echo "  Maintenance:"
	@echo "    make clean            Clean all build artifacts"
	@echo "====================================================================="

all: $(EXTENSIONS)

# Group build targets
hyp_group: $(HYP_GROUP)
ss_group:  $(SS_GROUP)
sv_group:  $(SV_GROUP)
sd_group:  $(SD_GROUP)
sm_group:  $(SM_GROUP)
pmp_group: $(PMP_GROUP)
cfi_group: $(CFI_GROUP)
zpm_group: $(ZPM_GROUP)
cmo_group: $(CMO_GROUP)
zi_group:  $(ZI_GROUP)
zc_group:  $(ZC_GROUP)
zk_group:  $(ZK_GROUP)
za_group:  $(ZA_GROUP)

$(EXTENSIONS):
	$(MAKE) -C $@ $(MAKE_VARS)

clean:
	@for ext in $(EXTENSIONS); do \
		if [ -d $$ext ]; then \
			$(MAKE) -C $$ext clean $(MAKE_VARS); \
		else \
			echo "Skipping $$ext (directory not found)"; \
		fi; \
	done
	find common -name "*.o" -type f -delete
	# Clean residual build artifacts in directories not listed in EXTENSIONS
	find . -name "*.o" -not -path "./.git/*" -type f -delete
	find . -name "*.d" -not -path "./.git/*" -type f -delete
	find . -name "*.asm" -not -path "./.git/*" -type f -delete
	find . -name "*.sym" -not -path "./.git/*" -type f -delete
	find . -name "*.elf" -not -path "./.git/*" -type f -delete
	find . -name "*.bin" -not -path "./.git/*" -not -path "./SPEC/*" -type f -delete
	find . -name "*.trace" -not -path "./.git/*" -type f -delete
	find . -name "*.rvvi" -not -path "./.git/*" -type f -delete

# Build and run all extensions on Sail simulator (sequentially)
sail:
	@for ext in $(EXTENSIONS); do \
		echo "========== Sail: $$ext =========="; \
		$(MAKE) -C $$ext sail $(MAKE_VARS) || exit 1; \
	done

# Build and run a single extension on Sail (e.g., make sail-pmp)
$(SAIL_TARGETS):
	$(MAKE) -C $(patsubst sail-%,%,$@) sail $(MAKE_VARS)

# Build and run all extensions on Spike simulator (sequentially)
spike:
	@for ext in $(EXTENSIONS); do \
		echo "========== Spike: $$ext =========="; \
		$(MAKE) -C $$ext spike $(MAKE_VARS) || exit 1; \
	done

# Build and run a single extension on Spike (e.g., make spike-pmp)
$(SPIKE_TARGETS):
	$(MAKE) -C $(patsubst spike-%,%,$@) spike $(MAKE_VARS)

# Build and run a single extension on QEMU (e.g., make qemu-sv39)
$(QEMU_TARGETS):
	$(MAKE) -C $(patsubst qemu-%,%,$@) qemu $(MAKE_VARS)

# Build and run all extensions on Whisper simulator (sequentially)
whisper:
	@for ext in $(EXTENSIONS); do \
		echo "========== Whisper: $$ext =========="; \
		$(MAKE) -C $$ext whisper $(MAKE_VARS) || exit 1; \
	done

# Build and run a single extension on Whisper (e.g., make whisper-Sv39)
$(WHISPER_TARGETS):
	$(MAKE) -C $(patsubst whisper-%,%,$@) whisper $(MAKE_VARS)
