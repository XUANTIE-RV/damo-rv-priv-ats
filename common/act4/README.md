# ACT4 trap handler

The trap handler from [riscv-arch-test](https://github.com/riscv/riscv-arch-test)
(`act4` branch), with the glue needed to run this suite on it, including T-SBI
privilege switching.

```sh
make spike-Hypervisor_Exceptions CONFIG=spike-rv64-max TRAP_HANDLER=act4
```

Without `TRAP_HANDLER=act4` none of this is compiled and the suite runs on its
own handler exactly as before.

## Why

Certification targets RVA23, which does not require a conforming M-mode. A test
that switches privilege by setting `mstatus.MPP` and executing `mret` in M-mode
code it supplies itself — which is what `ecall_args[] + ECALL_GOTO_PRIV` does —
cannot run there. The T-SBI is how ACT4 solves that, and the handler is where it
is implemented, so the handler comes with it.

## What is here

| Path | |
|---|---|
| `rvtest_trap_handler.h` | ACT4's handler, plus the changes described below |
| `signature.h`, `utils.h`, `encoding.h` | ACT4 support headers, unmodified |
| `act4_trap.S` | Instantiates the trampolines, save areas, prolog and epilog; environment symbols |
| `act4_glue.c` | Capability probes, T-SBI privilege switching, reporting helpers |
| `act4_trap_entry.h` | The trapping state the handler captures |
| `act4_tsbi.h` | T-SBI calling convention, C side |
| `act4.h`, `act4_config.h` | API and build settings |
| `act4_ab_compare.sh` | Runs suites under both handlers and diffs the verdicts |

Imported from `riscv-arch-test` commit
`74efcaac81f48f437f58868771daf2ed2776d422` (2026-08-19), files
`tests/env/{rvtest_trap_handler.h,signature.h,utils.h,encoding.h}`.

Local changes to `rvtest_trap_handler.h` and `signature.h` are exactly the
blocks guarded by `#ifdef RVTEST_ASSERTION_MODE` — 14 of them, all additions,
no upstream line removed. To see them against upstream, fetch that commit and
diff. Building without `RVTEST_ASSERTION_MODE` gives stock ACT4 behaviour.

## How a trap flows

```
   trap
     |
     v
  ACT4 trampoline          vector spreader -> per-cause stub
     |                     swap sp <-> xSCRATCH, save T1..T6
     v
  common_Xentry
     |                     capture cause, epc, tval, status,
     |                     mtval2/htval, mtinst/htinst, hstatus
     v
  ecall?  --yes-->  armed?  --yes-->  T-SBI: GOTO_M/S/U/VS/VU
     |                 |                     ECALL_TEST
     |                 |                     CSR_ACCESS
     |                 |                          |
     |                 |                          +--> xret, done
     |                 +--no--+
     v                        |
  trap signature words <------+
     |                     computed as always, stored to a shadow
     |                     buffer instead of the signature region
     v
  m_trap_handler / s_trap_handler        common/trap.c
     |                     expected? record it. interrupt? quiesce it.
     |                     set the resume PC. unexpected? fail.
     v
  mret / sret
```

The handler owns the architecture of a trap — entry, register discipline,
T-SBI, signature computation. The suite owns the policy — what was expected,
where to resume, what is fatal. `common/trap.c` decides exactly what it decided
before; it just runs a layer further in.

## Assertion mode

`RVTEST_ASSERTION_MODE` is on by default here. The suite checks state with
on-device assertions and has no reference signature, so a signature would have
nothing to compare against and its pointer would eventually overrun.

| | ACT4 default | Assertion mode |
|---|---|---|
| Signature words | written to the signature region, pointer advances | written to a shadow buffer, pointer does not move |
| Trapping state | not captured for C | captured before any dispatch rewrites xEPC |
| `a0 == 0` on illegal instruction | means "return in M-mode" | ordinary trap; the suite raises these as stimulus |
| ecall dispatch | every ecall is a candidate T-SBI call | only an armed one |
| xEPC in the signature | relocated to a segment offset | recorded raw |
| Resume PC | `ra` on a fetch fault, else advanced by instruction width | left to the policy layer |
| Interrupts | clears `xIE[cause]`, dispatches a clearing routine | reads `xIE`/`xIP` unchanged; the suite quiesces the source |
| Prolog `xSATP` | replaced with a handler-owned identity table | left alone; the suite owns translation |
| HS trap values | words 4/5 written on the M path only | written on the HS path too |

Two of those are worth more than a table row.

**The resume PC.** ACT4 measures the trapping instruction's width by loading the
halfword at `xEPC` with `mstatus.MPRV|SUM|MXR` set, in the trapped context's
translation regime. On a two-stage trap out of VS or VU that load goes through
both stages and can fault, nesting a trap inside the handler while a trap
expectation is still armed — and the nested fault would be recorded instead of
the one under test. The policy layer sets the resume PC from the captured state
instead.

**The arming gate.** `Hypervisor_Exceptions` asserts on ecall traps — causes
8/10 with SPV, GVA and `htinst` — and does not control `a0` at those sites.
Dispatching on `a0` alone turns such a trap into a mode switch whenever `a0`
happens to hold 1..5, or aborts the run through the "no dispatch-table entry"
path, and the test cannot tell either happened. So `act4_tsbi_call()` sets a
word the handler consumes; anything else is an ordinary trap.

This is the opposite polarity to decision 9.5 of
`DOCS/framework/tsbi_act4_adaptation_plan.md`, which arms the escape rather than
the call. That way needs every raw-ecall site annotated; this way needs no test
changes. Worth settling before this goes upstream.

## Checking a suite

```sh
common/act4/act4_ab_compare.sh Hypervisor_Exceptions Hypervisor_CSR Sv39x4_Sv39
```

Runs each suite under both handlers and diffs the per-test verdicts.
`identical` means the ACT4 handler is a drop-in replacement for that suite.

## Limits

- `vstvec` is left to the tests, which install and check their own VS-mode
  vectors (`common/hyp/hyp_vs_trap.c`).
- `stvec` gets the S-mode handler. ACT4's HS-mode instantiation records `htval`
  where this suite asserts on `stval`; `htval` and `htinst` are captured
  separately at trap entry. Neither the HS nor the VS instantiation is emitted.
- A test that installs its own `mtvec` — the Ssdbltrp suite's
  `smdbltrp_m_trap_entry` — keeps it for the duration and does not get the
  handler's services meanwhile. It still behaves correctly: the suite's entry
  stubs in `common/trap_asm.S` capture the trapping state themselves.
- `act4_cap_hyp` latches `misa.H` at boot, since `misa` is not readable below
  M-mode. A test that clears `misa.H` and then takes an HS-mode trap would still
  have the handler read `htval`. The M-side gate is a live `misa.H` test.
- `RVTEST_SIGNATURE_MODE=1` does not link: it needs `rvtest_<MODE>root_pg_tbl`
  and the `sv_<MODE><field>_str` tables, which come with ACT4 test scaffolding
  this suite replaces with its own. The build stops with a message saying so.
