# =====================================================================
# Top-level Makefile for RISC-V Privilege Test Framework
#
# Usage:
#   make                    # Print help
#   make pmp_group          # Build all PMP-related extensions
#   make sv_group           # Build all SV-related extensions
#   make ss_group           # Build all SS-related extensions
#   make hyp_group          # Build all Hypervisor extensions
#   make all                # Build ALL extensions
#   make pmp                # Build a single extension
#   make spike              # Run all on Spike
#   make spike-pmp          # Run one on Spike
#   make sail-pmp           # Run one on Sail
#   make qemu-pmp           # Run one on QEMU
#   make clean              # Clean all
# =====================================================================

# Extension groups
PMP_GROUP = pmp smepmp spmp pmp_sv39 pmp_sv48 pmp_sv57
SV_GROUP  = Sv39 Sv48 Sv57 Svbare Svade Svadu Svnapot Svinval Svpbmt Svvptc Svrsw60t59b
SS_GROUP  = Ssccptr Sscofpmf Sscounterenw Ssstateen Sstc Sstvala Sstvecd Ssu64xl
SM_GROUP  = Smstateen smrnmi
HYP_GROUP = Sv39x4 Sv48x4 Sv57x4 \
            Sv39_Sv39x4 Sv39_Sv48x4 Sv39_Sv57x4 \
            Sv48_Sv39x4 Sv48_Sv48x4 Sv48_Sv57x4 \
            Sv57_Sv39x4 Sv57_Sv48x4 Sv57_Sv57x4 \
			Shgatpa Shtvala Shcounterenw Shvstvecd Shvstvala Shvsatpa Shlcofideleg \
			Hypervisor Sha Hypervisor_Sscounterenw Hypervisor_Svinval Hypervisor_Sstc
INT_GROUP = aia_aplic aia_imsic aia_smaia aia_iommu aia_hypervisor

# All extensions (union of groups + ungrouped)
EXTENSIONS = $(PMP_GROUP) $(SV_GROUP) $(SS_GROUP) $(SM_GROUP) $(HYP_GROUP) $(INT_GROUP) zpm

# Forward all variables to sub-makes
MAKE_VARS = $(if $(XLEN),XLEN=$(XLEN)) \
            $(if $(CONFIG),CONFIG=$(CONFIG)) \
            $(if $(CROSS_COMPILER),CROSS_COMPILER=$(CROSS_COMPILER)) \
            $(if $(LOG_LEVEL),LOG_LEVEL=$(LOG_LEVEL)) \
            $(if $(SAIL),SAIL=$(SAIL)) \
            $(if $(SPIKE),SPIKE=$(SPIKE)) \
            $(if $(SPIKE_ISA),SPIKE_ISA=$(SPIKE_ISA))

# Generate sail-<ext>, spike-<ext>, and qemu-<ext> targets
SAIL_TARGETS  = $(addprefix sail-,$(EXTENSIONS))
SPIKE_TARGETS = $(addprefix spike-,$(EXTENSIONS))
QEMU_TARGETS  = $(addprefix qemu-,$(EXTENSIONS))

.PHONY: help all clean sail spike qemu \
        pmp_group sv_group ss_group hyp_group \
        $(EXTENSIONS) $(SAIL_TARGETS) $(SPIKE_TARGETS) $(QEMU_TARGETS)

# Default target: print usage help
.DEFAULT_GOAL := help

help:
	@echo "====================================================================="
	@echo "  RISC-V Privilege Test Framework - Build System"
	@echo "====================================================================="
	@echo ""
	@echo "  Group targets:"
	@echo "    make pmp_group        Build PMP extensions:  $(PMP_GROUP)"
	@echo "    make sv_group         Build SV extensions:   $(SV_GROUP)"
	@echo "    make ss_group         Build SS extensions:   $(SS_GROUP)"
	@echo "    make hyp_group        Build HYP extensions:  $(HYP_GROUP)"
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
	@echo ""
	@echo "  Options:"
	@echo "    XLEN=32|64            Architecture (default: 64)"
	@echo "    CROSS_COMPILER=...    Toolchain prefix"
	@echo "    CONFIG=...            Target configuration"
	@echo "    LOG_LEVEL=1-6         Verbosity (default: 3)"
	@echo ""
	@echo "  Maintenance:"
	@echo "    make clean            Clean all build artifacts"
	@echo "====================================================================="

all: $(EXTENSIONS)

# Group build targets
pmp_group: $(PMP_GROUP)
sv_group:  $(SV_GROUP)
ss_group:  $(SS_GROUP)
hyp_group: $(HYP_GROUP)

$(EXTENSIONS):
	$(MAKE) -C $@ $(MAKE_VARS)

clean:
	@for ext in $(EXTENSIONS); do \
		$(MAKE) -C $$ext clean $(MAKE_VARS); \
	done
	find common -name "*.o" -type f -delete
	# Clean residual build artifacts in directories not listed in EXTENSIONS
	find . -name "*.asm" -not -path "./.git/*" -type f -delete
	find . -name "*.sym" -not -path "./.git/*" -type f -delete
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
