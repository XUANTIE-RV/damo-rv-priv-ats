# Whisper RISC-V simulator platform configuration

# Cross compiler
CROSS_COMPILER ?= riscv64-unknown-elf-

# Whisper simulator
WHISPER ?= whisper

# Memory base (Whisper default, matches reset_vec in whisper.json)
MEM_BASE ?= 0x80000000
