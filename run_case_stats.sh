#!/bin/bash
# Parameterized HYP_GROUP Case-Level Test Runner
#
# Usage: ./run_case_stats.sh [-t TOOLCHAINS] [-s SIMULATORS] [-l SUITES] [-h]
#   -t TOOLCHAINS : space-separated toolchains, quoted, e.g. -t "gcc clang"
#   -s SIMULATORS : space-separated simulators, quoted, e.g. -s "qemu spike"
#   -l SUITES     : space-separated suite group names and/or suite dirs,
#                   quoted, e.g. -l "hyp ss" or -l "Ss_CSR pmp"
#
# Defaults (no arguments): loop all toolchains (gcc clang), all simulators
# (qemu spike) and all suites.

SCRIPT_DIR="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

ALL_TOOLCHAINS="gcc clang"
ALL_SIMULATORS="qemu spike"
LOGDIR="${SCRIPT_DIR}/statistics"

# ---------------------------------------------------------------------
# Suite group definitions
# ---------------------------------------------------------------------
# Supervisor extensions
EXT_SS="Ss_CSR Ss_Exceptions Ss_Interrupts Ssccfg Ssccptr Sscofpmf Sscounterenw Sscsrind Ssctr Ssdbltrp Ssstateen Sstc Sstvala Sstvecd Ssu64xl"
EXT_SV="Sv39 Sv48 Sv57 Svbare Svade Svadu Svnapot Svinval Svpbmt Svvptc Svrsw60t59b"

# Machine extensions
EXT_SM="Sm_CSR Sm_Exceptions Sm_Interrupts Smcdeleg Smcntrpmf Smcsrind Smctr Smdbltrp Smstateen"
EXT_PMP="pmp pmp_sv39 pmp_sv48 pmp_sv57"
# cfi extensions
EXT_CFI="cfi.Zicfilp cfi.Zicfiss"
# pointer masking extensions
EXT_ZPM="zpm.Smmpm zpm.Smnpm zpm.Ssnpm"
# cmo extensions
EXT_CMO="cmo.base cmo.Zicbom cmo.Zicbop cmo.Zicboz"

# Hypervisor extensions
EXT_HYP="Hypervisor Sha Shcounterenw Shgatpa Shlcofideleg Shtvala Shvstvala Shvsatpa"

# Hypervisor translation modes
EXT_HYP_VM="Sv39x4 Sv48x4 Sv57x4 Sv39x4_Sv39 Sv39x4_Sv48 Sv39x4_Sv57 Sv48x4_Sv39 Sv48x4_Sv48 Sv48x4_Sv57 Sv57x4_Sv39 Sv57x4_Sv48 Sv57x4_Sv57"

# Hypervisor combined
EXT_HYP_SM="Hypervisor_Smcntrpmf  Hypervisor_Smcsrind  Hypervisor_Smmpm  Hypervisor_Smnpm  Hypervisor_Smstateen"
EXT_HYP_SS="Hypervisor_Ssccptr  Hypervisor_Sscsrind  Hypervisor_Ssdbltrp  Hypervisor_Ssnpm  Hypervisor_Ssqosid  Hypervisor_Ssstateen  Hypervisor_Sstc  Hypervisor_Sstvala"
EXT_HYP_SV="Hypervisor_Svadu  Hypervisor_Svinval  Hypervisor_Svnapot  Hypervisor_Svpbmt"
EXT_HYP_ZI="Hypervisor_Zicbom  Hypervisor_Zicbop  Hypervisor_Zicboz  Hypervisor_Zicfilp  Hypervisor_Zicfiss  Hypervisor_Zkr"

ALL_SUITES="${EXT_HYP} ${EXT_HYP_VM} ${EXT_HYP_SM} ${EXT_HYP_SS} ${EXT_HYP_SV} ${EXT_HYP_ZI} ${EXT_SS} ${EXT_SV} ${EXT_SM} ${EXT_PMP} ${EXT_CFI} ${EXT_ZPM} ${EXT_CMO}"

usage()
{
    echo "Usage: $0 [-t TOOLCHAINS] [-s SIMULATORS] [-l SUITES] [-h]"
    echo "  -t TOOLCHAINS : space-separated toolchains, quoted (default: \"${ALL_TOOLCHAINS}\")"
    echo "  -s SIMULATORS : space-separated simulators, quoted (default: \"${ALL_SIMULATORS}\")"
    echo "  -l SUITES     : space-separated suite groups or suite dirs, quoted"
    echo "                  groups: ${ALL_SUITES} all"
    echo "                  default: \"all\""
    echo "  -h            : show this help"
    exit 1
}

# Map a suite argument (group name or suite directory) to a list of dirs
resolve_suite()
{
    local s="$1"
    case "$s" in
        all)  echo "${ALL_SUITES}" ;;
        hyp)  echo "${EXT_HYP} ${EXT_HYP_VM} ${EXT_HYP_SM} ${EXT_HYP_SS} ${EXT_HYP_SV} ${EXT_HYP_ZI}" ;;
        ss)   echo "${EXT_SS}" ;;
        sv)   echo "${EXT_SV}" ;;
        sm)   echo "${EXT_SM}" ;;
        *)
            if [ -d "${SCRIPT_DIR}/${s}" ]; then
                echo "$s"
            else
                echo "ERROR: unknown suite or group: ${s}" >&2
                usage
            fi
            ;;
    esac
}

# ---------------------------------------------------------------------
# Parse arguments
# ---------------------------------------------------------------------
TOOLCHAINS=""
SIMULATORS=""
SUITE_ARGS=""

while getopts "t:s:l:h" opt; do
    case "$opt" in
        t) TOOLCHAINS="$OPTARG" ;;
        s) SIMULATORS="$OPTARG" ;;
        l) SUITE_ARGS="$OPTARG" ;;
        h) usage ;;
        *) usage ;;
    esac
done

TOOLCHAINS=${TOOLCHAINS:-$ALL_TOOLCHAINS}
SIMULATORS=${SIMULATORS:-$ALL_SIMULATORS}
SUITE_ARGS=${SUITE_ARGS:-all}

# Resolve suite arguments into the final extension list (deduplicated)
EXTENSIONS=""
for s in $SUITE_ARGS; do
    EXTENSIONS="${EXTENSIONS} $(resolve_suite "$s")"
done
EXTENSIONS=$(echo "$EXTENSIONS" | tr ' ' '\n' | grep -v '^$' | awk '!seen[$0]++' | tr '\n' ' ')

echo "Toolchains : ${TOOLCHAINS}"
echo "Simulators : ${SIMULATORS}"
echo "Suites     : ${EXTENSIONS}"
echo ""

cd "$SCRIPT_DIR"
mkdir -p "$LOGDIR"

# ---------------------------------------------------------------------
# Run all (simulator, toolchain) combinations over the extension list
# ---------------------------------------------------------------------
for SIMULATOR in $SIMULATORS; do
    for TOOLCHAIN in $TOOLCHAINS; do
        SUITE_TAG=$(echo "$SUITE_ARGS" | tr ' ' '_')
        RESULT_FILE="${LOGDIR}/${SIMULATOR}_${TOOLCHAIN}_${SUITE_TAG}_case_summary.log"
        CSV_FILE="${LOGDIR}/${SIMULATOR}_${TOOLCHAIN}_${SUITE_TAG}_case_summary.csv"

        echo "=== Starting Extension tests on ${SIMULATOR} with ${TOOLCHAIN} ===" | tee "$RESULT_FILE"
        echo "Time: $(date)" | tee -a "$RESULT_FILE"
        echo "" | tee -a "$RESULT_FILE"

        # Initialize CSV file with header
        echo "CASE,TOTAL,PASS,FAIL,SKIP" > "$CSV_FILE"

        for ext in $EXTENSIONS; do
            LOGFILE="${LOGDIR}/${SIMULATOR}_${TOOLCHAIN}_${ext}.log"
            echo "Testing ${ext} on ${SIMULATOR} with ${TOOLCHAIN}"

            # Clean first
            make -C "$ext" clean > /dev/null 2>&1

            # Run test
            make -C "$ext" TOOLCHAIN="$TOOLCHAIN" "$SIMULATOR" > "$LOGFILE" 2>&1

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
        echo "All extension tests completed on ${SIMULATOR} with ${TOOLCHAIN}"
        echo ""
    done
done
