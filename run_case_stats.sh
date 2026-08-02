#!/bin/bash
# Simple HYP_GROUP Case-Level Test Runner

SCRIPT_DIR="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

SIMULATOR="${SIMULATOR:-qemu}"
TOOLCHAIN="${TOOLCHAIN:-gcc}"
SUITE="${1:-all}"
LOGDIR="${SCRIPT_DIR}/statistics"
RESULT_FILE="${LOGDIR}/${SIMULATOR}_${TOOLCHAIN}_${SUITE}_case_summary.log"
CSV_FILE="${LOGDIR}/${SIMULATOR}_${TOOLCHAIN}_${SUITE}_case_summary.csv"

# Supervisor extensions
EXT_SS="Ss_CSR Ss_Exceptions Ss_Interrupts Ssccfg Ssccptr Sscofpmf Sscounterenw Sscsrind Ssctr Ssdbltrp Ssstateen Sstc Sstvala Sstvecd Ssu64xl"
EXT_SV="Sv39 Sv48 Sv57 Svbare Svade Svadu Svnapot Svinval Svpbmt Svvptc Svrsw60t59b"

# Machine extensions
EXT_SM="Sm_CSR Sm_Exceptions Sm_Interrupts Smcdeleg Smcntrpmf Smcsrind Smctr Smdbltrp Smstateen"
EXT_PMP="pmp pmp_sv39 pmp_sv48 pmp_sv57"
# cfi extensions
EXT_CFI="cfi.Zicfilp cfi.Zicfilp"
# pointer masking extensions
EXT_ZPM="zpm.Smmpm zpm.Smnpm zpm.Ssnpm"
# cmo extensions
EXT_CMO="cmo.base cmo.Zicbom cmo.Zicbop cmo.Zicboz"


# Hypervisor extensions Shvstvecd
EXT_HYP="Hypervisor Sha Shcounterenw Shgatpa Shlcofideleg Shtvala Shvstvala Shvsatpa"

# Hypervisor translation modes
EXT_HYP_VM="Sv39x4 Sv48x4 Sv57x4 Sv39x4_Sv39 Sv39x4_Sv48 Sv39x4_Sv57 Sv48x4_Sv39 Sv48x4_Sv48 Sv48x4_Sv57 Sv57x4_Sv39 Sv57x4_Sv48 Sv57x4_Sv57"

# Hypervisor combined
EXT_HYP_SM="Hypervisor_Smcntrpmf  Hypervisor_Smcsrind  Hypervisor_Smmpm  Hypervisor_Smnpm  Hypervisor_Smstateen"
EXT_HYP_SS="Hypervisor_Ssccptr  Hypervisor_Sscsrind  Hypervisor_Ssdbltrp  Hypervisor_Ssnpm  Hypervisor_Ssqosid  Hypervisor_Ssstateen  Hypervisor_Sstc  Hypervisor_Sstvala"
EXT_HYP_SV="Hypervisor_Svadu  Hypervisor_Svinval  Hypervisor_Svnapot  Hypervisor_Svpbmt"
EXT_HYP_ZI="Hypervisor_Zicbom  Hypervisor_Zicbop  Hypervisor_Zicboz  Hypervisor_Zicfilp  Hypervisor_Zicfiss  Hypervisor_Zkr"

# Compose final extension list based on SUITE parameter
if [ "$SUITE" = "hyp" ]; then
    EXTENSIONS="${EXT_HYP} ${EXT_HYP_VM} ${EXT_HYP_SM} ${EXT_HYP_SS} ${EXT_HYP_SV} ${EXT_HYP_ZI}"
else
    EXTENSIONS="${EXT_HYP} ${EXT_HYP_VM} ${EXT_HYP_SM} ${EXT_HYP_SS} ${EXT_HYP_SV} ${EXT_HYP_ZI} ${EXT_SS} ${EXT_SV} ${EXT_SM} ${EXT_PMP} ${EXT_CFI} ${EXT_ZPM} ${EXT_CMO}"
fi



cd $SCRIPT_DIR
mkdir -p $LOGDIR
echo "=== Starting Extension tests on ${SIMULATOR} ===" | tee "$RESULT_FILE"
echo "Time: $(date)" | tee -a "$RESULT_FILE"
echo "" | tee -a "$RESULT_FILE"

# Initialize CSV file with header
echo "CASE,TOTAL,PASS,FAIL,SKIP" > "$CSV_FILE"

for ext in $EXTENSIONS; do
    LOGFILE="${LOGDIR}/${SIMULATOR}_${ext}.log"
    echo "Testing ${ext}... on ${SIMULATOR}... with ${TOOLCHAIN}"

    # Clean first
    make -C "$ext" clean > /dev/null 2>&1

    # Run test with timeout
    make -C "$ext" TOOLCHAIN=$TOOLCHAIN ${SIMULATOR} > "$LOGFILE" 2>&1


    # Parse results from summary block (tr -d '\r\n' strips both CR and LF)
    TOTAL_COUNT=$(grep -E "^\s*Total:" "$LOGFILE" 2>/dev/null | awk '{print $2}' | head -n 1 | tr -d '\r\n')
    PASS_COUNT=$(grep -E "^\s*Passed:" "$LOGFILE" 2>/dev/null | awk '{print $2}' | head -n 1 | tr -d '\r\n')
    FAIL_COUNT=$(grep -E "^\s*Failed:" "$LOGFILE" 2>/dev/null | awk '{print $2}' | head -n 1 | tr -d '\r\n')
    SKIP_COUNT=$(grep -E "^\s*Skipped:" "$LOGFILE" 2>/dev/null | awk '{print $2}' | head -n 1 | tr -d '\r\n')

    # Default to 0 if empty
    TOTAL_COUNT=${TOTAL_COUNT:-0}
    PASS_COUNT=${PASS_COUNT:-0}
    FAIL_COUNT=${FAIL_COUNT:-0}
    SKIP_COUNT=${SKIP_COUNT:-0}

    echo "${ext}: TOTAL=${TOTAL_COUNT} PASS=${PASS_COUNT} FAIL=${FAIL_COUNT} SKIP=${SKIP_COUNT}" | tee -a "$RESULT_FILE"
    echo "${ext},${TOTAL_COUNT},${PASS_COUNT},${FAIL_COUNT},${SKIP_COUNT}" >> "$CSV_FILE"
    echo ""
done

echo "" | tee -a "$RESULT_FILE"
echo "=== Completed at $(date) ===" | tee -a "$RESULT_FILE"
echo "All extension tests completed on ${SIMULATOR} "
