#!/bin/bash
# =====================================================================
# serial_monitor.sh - Monitor HAPS-XIAOHUI serial output
#
# Connects to the HAPS UART via telnet and logs output to a file.
#
# Usage:
#   ./serial_monitor.sh              # Default log: svinval_haps.log
#   ./serial_monitor.sh mylog.log    # Custom log file
#
# Press Ctrl+] then 'quit' to exit telnet, or Ctrl+C to kill.
# =====================================================================

HAPS_HOST="10.63.42.218"
HAPS_PORT="5000"
LOG_FILE="${1:-svinval_haps.log}"

echo "====================================================="
echo "  HAPS Serial Monitor"
echo "====================================================="
echo "  Host:     $HAPS_HOST"
echo "  Port:     $HAPS_PORT"
echo "  Log file: $LOG_FILE"
echo ""
echo "  Press Ctrl+] then type 'quit' to exit telnet."
echo "  Or press Ctrl+C to terminate."
echo "====================================================="
echo ""
echo "Connecting..."
echo ""

# Use telnet with tee to log output
# The script command captures the full terminal session
telnet "$HAPS_HOST" "$HAPS_PORT" 2>&1 | tee -a "$LOG_FILE"
