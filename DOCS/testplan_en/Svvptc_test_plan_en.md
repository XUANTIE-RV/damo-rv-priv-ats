**[中文](../testplan/Svvptc_test_plan.md) | English**

# Svvptc Extension Test Plan

This document describes the test plan for the Svvptc (Obviating Memory-Management Instructions after Marking PTEs Valid) extension. The Svvptc extension specifies that after a hart explicitly stores a leaf/non-leaf PTE with the Valid bit transitioning **from 0 to 1**, the operating system **no longer needs** to execute address translation cache synchronization instructions such as `SFENCE.VMA` / `SINVAL.VMA`; the PTE update will become visible to subsequent implicit accesses (address translations) by that hart within a **bounded timeframe**.

---

## SPEC Sections Covered by This Document

This plan is based on the following official RISC-V specifications (local paths):

- `SPEC/riscv-isa-manual/src/priv/svvptc.adoc` — Svvptc extension definition: an explicit store that sets the Valid bit of a leaf/non-leaf PTE from 0 to 1 becomes visible to subsequent implicit accesses within a bounded timeframe; the NOTE explains that the OS may omit sfence and that an "occasional gratuitous additional page fault" is tolerated
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — Sv39 virtual address translation algorithm (translation halts and raises the corresponding page-fault type when any intermediate PTE or leaf PTE has V=0)
- `SPEC/riscv-isa-manual/src/priv/svinval.adoc` — SFENCE.VMA / SINVAL.VMA semantics (used as the boundary reference for "reverse scenarios still require sfence"; not within the scope of this plan)

Official repository:

- https://github.com/riscv/riscv-isa-manual (the above path files within the repository)

---

## Overview

The RISC-V Privileged Specification requires that when software modifies a PTE in memory, it must execute `SFENCE.VMA` or `SINVAL.VMA` to synchronize the hart's address translation cache (TLB / Page Walk Cache); otherwise, subsequent accesses may still use stale translations.

The Svvptc extension relaxes the synchronization requirement for **one specific direction**:

- **Covered scenario (Svvptc guarantee)**: The PTE Valid bit `V: 0 -> 1`, including both leaf and non-leaf PTEs. Instructions such as `SFENCE.VMA` become **redundant**; hardware guarantees that the update becomes visible to subsequent implicit accesses within a bounded timeframe, at the cost of **possibly incurring an occasional gratuitous additional page fault** (spec NOTE: "occasional gratuitous additional page fault").
- **Uncovered scenarios (sfence still required)**: Any **other** form of PTE update, including `V: 1 -> 0`, permission downgrade (e.g., RW -> R), PA remapping (PPN change), software clearing of A/D bits, etc. These scenarios still require `SFENCE.VMA` / `SINVAL.VMA` and are independently covered by `Svinval_test_plan.md`.

This test plan focuses on the behavior of the specification in a **single hart, single thread** scenario: without invoking sfence.vma, it verifies that after a V=0->1 modification the access **eventually** succeeds (within the bounded retry limit), and uses "immediate success with sfence" as a baseline comparison.

---

## Covered Specification Points

The table below lists the specification points covered by this plan. Entries with the `norm:` prefix are official SPEC normative rule tags; entries without the prefix are specification points summarized by this plan from the SPEC text.

| Norm ID | Original Text | Description |
|---------|---------------|-------------|
| `norm:Svvptc_explicit_stores_update_valid_bit` | When the Svvptc extension is implemented, explicit stores by a hart that update the Valid bit of leaf and/or non-leaf PTEs from 0 to 1 and are visible to a hart will eventually become visible within a bounded timeframe to subsequent implicit accesses by that hart to such PTEs. | When Svvptc is implemented, explicit stores by a hart that update the Valid bit of leaf and/or non-leaf PTEs from 0 to 1 will eventually become visible, within a bounded timeframe, to that hart's subsequent implicit accesses to such PTEs. |
| `Svvptc_bounded_time_eventual_visibility` | Observable quantification of the "bounded timeframe": occasional gratuitous page-faults are allowed, but the access must succeed within a bounded number of retries. | Quantify and verify the bounded time via "in-place retry after a page-fault + a maximum retry limit": if it still fails after exceeding the limit, it is judged non-compliant with Svvptc. |
| `Svvptc_reverse_boundary_baseline` | The traditional path with an explicit sfence.vma added after V:0->1 must succeed immediately. | Use "immediate success with sfence" as a baseline comparison to rule out false positives caused by incorrect PTE setup in the test case itself. |

---

## Out of Scope

- **Reverse scenarios** (V: 1->0, permission downgrade, PPN remapping, A/D software clearing): Svvptc does **not** guarantee that such updates can omit sfence; these scenarios are covered by `Svinval_test_plan.md`.
- **Hypervisor two-stage translation**: Svvptc behavior under VS-stage / G-stage is covered by `Hypervisor_Sv_test_plan.md` and related Hypervisor plans.
- **Sv32 / Sv48 / Sv57 modes**: This plan takes Sv39 as the primary coverage target; V:0->1 semantics are identical under the other Sv modes.
- **Multi-hart consistency**: The Svvptc specification explicitly constrains only the visibility between explicit stores and implicit accesses on the **same hart**; cross-hart visibility requires mechanisms such as IPI + sfence, which are beyond the scope of Svvptc.
- **Composite interactions with Svinval / Svadu**: Covered by each extension's independent plan.

---

## Design Key Points

### 1. Bounded-Time Verification Strategy (Retry + Limit)

Svvptc allows "occasional gratuitous page faults," so the **first** implicit access after a V=0->1 modification may still trigger a page-fault. The test verifies "eventual visibility within bounded time" as follows:

1. The test code accesses the target VA in S-mode, expecting **eventual** success.
2. The S-mode local trap handler, after capturing a page-fault, **does not increment sepc**, but instead retries the same instruction in place while accumulating a retry count.
3. When the retry count exceeds the configured **maximum retry limit**, the trap handler directs control flow to an escape exit and writes back a failure flag.
4. The test case finally asserts: `access succeeded` and `retry count <= maximum retry limit`.

> [!NOTE]
> The maximum retry limit should be set to a sufficiently large conservative value (adjustable during the execution phase per the platform's observed distribution) to accommodate the multiple gratuitous page-faults a real implementation may produce; but it **must not be reduced to 0** -- reducing it to 0 is equivalent to mandating "first-access success," which violates the spec NOTE regarding tolerance of an "occasional gratuitous additional page fault."

### 2. No Active sfence.vma Invocation

All forward cases (Groups 1~4) **deliberately omit** `sfence.vma`, relying on Svvptc's "bounded-time eventual visibility" guarantee. This is the core premise of the test -- if an implementation incorrectly caches Invalid PTEs without an eviction mechanism, the case will retry indefinitely until it exceeds the limit and fails.

### 3. Direct Store to Modify the PTE

The test obtains the PTE slot pointer and then modifies the V bit with a plain store, corresponding to "explicit stores by a hart" in the specification. Within the same hart, a store to an address followed by a subsequent instruction read to the same address is automatically guaranteed visible by RVWMO's program order + same-address dependency; no additional `fence rw, rw` is appended after the PTE store (it neither replaces `sfence.vma` nor provides extra semantics for the Svvptc path).

### 4. Instruction Pre-placement Flow for Fetch Cases

A leaf page with X permission and initial V=0 **cannot** have instruction bytes written to it by any store (V=0 means neither readable nor writable). Therefore all fetch-type cases (SVVPTC-4K-03 / SVVPTC-2M-03 / SVVPTC-NL-03) must follow this three-step pre-placement flow:

1. **Pre-placement phase**: First temporarily map the target page with RW permission and V=1, write an executable instruction sequence ending with `ret`, then execute `sfence.vma` to synchronize.
2. **Reconfiguration phase**: Change the permission to V=0, X=1, A=1 (remove R/W, clear V), and execute `sfence.vma` to synchronize. **This step must use sfence** -- V:1->0 and permission downgrade are **not** within the Svvptc guarantee scope.
3. **Test phase**: Again set the V bit to 1 (**without** sfence), enter S-mode and invoke via a function pointer, using the retry mechanism to absorb gratuitous instruction page-faults; finally assert that the access succeeds within the bounded retry limit.

### 5. Baseline Comparison (Sanity Check)

Each core test group has a corresponding baseline in Group 5: the same PTE modification flow **with** an explicit `sfence.vma` added, verifying that the "traditional path not relying on Svvptc" must succeed on the first access (retry count = 0). This eliminates false positives such as "incorrect PTE setup in the test case itself" and increases test confidence.

### 6. Necessity of sfence in Multi-Round Switching

SVVPTC-4K-04 uses `sfence.vma` at the **end** of each round to clear the V bit **back to 0**. The sfence here is **mandatory**: V:1->0 is a PTE update **outside** the Svvptc guarantee scope; without sfence, the TLB may still hit the old V=1 translation, causing the "V=0 starting point" of the next round to not actually take effect. Only the V=0->1 transition at the **start** of each round deliberately omits sfence, corresponding to the Svvptc test objective.

---

## Test Groups

### Group 1: 4 KiB Leaf PTE V: 0->1 Eventual Visibility Without sfence

**Spec Reference**:
- `norm:Svvptc_explicit_stores_update_valid_bit`: Leaf PTE V: 0->1 becomes visible to subsequent implicit accesses within a bounded time.

**Test Scope**: Verify on 4 KiB leaf PTEs that all three implicit access types (load / store / instruction fetch) succeed within the maximum retry limit after V=0->1; and verify repeatability through a "multi-round V-bit switching + sfence clear back to 0" case.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVVPTC-4K-01 | 4K V=0->1 load eventually visible | 4 KiB PTE initial V=0, R=1, A=1, D=1; a store sets PTE.V=1 (no sfence); S-mode repeatedly loads until success | load succeeds; retry count <= the limit |
| SVVPTC-4K-02 | 4K V=0->1 store eventually visible | 4 KiB PTE initial V=0, R=1, W=1, A=1, D=1; a store sets PTE.V=1 (no sfence); S-mode repeatedly stores until success | store succeeds; retry count <= the limit |
| SVVPTC-4K-03 | 4K V=0->1 fetch eventually visible | 4 KiB PTE initial V=0, X=1, A=1; a store sets PTE.V=1 (no sfence); S-mode jumps to execute | fetch succeeds; retry count <= the limit |
| SVVPTC-4K-04 | 4K V-bit multi-round switching visibility | Repeat 8 rounds: `sfence.vma` clears V to 0 (V:1->0 is outside the Svvptc scope, **sfence required**) -> set V to 1 without sfence (V:0->1 is protected by Svvptc) -> load until success; the retry count is accumulated independently per round | Each round succeeds; each round retry count <= the limit |

---

### Group 2: 2 MiB Megapage Leaf PTE V: 0->1 Eventual Visibility

**Spec Reference**:
- `norm:Svvptc_explicit_stores_update_valid_bit`: The specification does not distinguish PTE levels; 2 MiB megapage leaf PTEs are equally applicable.

**Test Scope**: Verify on 2 MiB megapage leaf PTEs that all three access types (load / store / fetch) succeed within the maximum retry limit after V=0->1.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVVPTC-2M-01 | 2M V=0->1 load eventually visible | 2 MiB megapage initial V=0, R=1, A=1, D=1; store V=1 (no sfence); S-mode load | load succeeds; retry <= the limit |
| SVVPTC-2M-02 | 2M V=0->1 store eventually visible | 2 MiB megapage initial V=0, R=1, W=1, A=1, D=1; store V=1 (no sfence); S-mode store | store succeeds; retry <= the limit |
| SVVPTC-2M-03 | 2M V=0->1 fetch eventually visible | 2 MiB megapage initial V=0, X=1, A=1; store V=1 (no sfence); S-mode jump to execute | fetch succeeds; retry <= the limit |

---

### Group 3: 1 GiB Gigapage Leaf PTE V: 0->1 Eventual Visibility (Placeholder Skip)

**Spec Reference**:
- `norm:Svvptc_explicit_stores_update_valid_bit`: 1 GiB gigapage leaf PTEs are equally applicable.

**Test Scope**: Verify on 1 GiB gigapage leaf PTEs whether load / store succeed within a bounded retry after V=0->1.

> [!IMPORTANT]
> **Testability note**: To verify the Svvptc V=0->1 semantics on a 1 GiB gigapage leaf PTE, the code segment, stack, page table pool, and UART MMIO required to run the test must all be placed **outside** the 1 GiB subspace under test -- otherwise, once that 1 GiB leaf PTE is at V=0, instruction fetch / stack access / page table access would all fail, making the test impossible to complete. The contiguous physical memory available in the current test environment is insufficient to simultaneously provide an "independent 1 GiB hosting region" and the "1 GiB subspace under test," so this dimension is explicitly recorded as skipped via `TEST_SKIP` with its reason. The specification coverage dimension is jointly fulfilled by Group 1 (4K) + Group 2 (2M) + Group 4 (non-leaf); a 1 GiB gigapage and a 2 MiB megapage are equivalent in Svvptc semantics (both are leaf-PTE V:0->1, and the specification text `leaf and/or non-leaf PTEs from 0 to 1` does not distinguish the specific level of the leaf), constituting no gap in specification coverage.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVVPTC-1G-01 | 1G V=0->1 load placeholder (skip) | Record the skip reason (needs a region independent of the 1 GiB subspace under test to host code/stack) | `[SKIP]` output, not counted as a failure |
| SVVPTC-1G-02 | 1G V=0->1 store placeholder (skip) | Same as SVVPTC-1G-01 | `[SKIP]` output, not counted as a failure |

> [!NOTE]
> The purpose of retaining both SVVPTC-1G-01 / -02 `TEST_SKIP` cases: to explicitly document in the test report that "the 1G dimension is deliberately skipped and the reason why," preventing future maintainers from mistakenly assuming it was an oversight -- this is a definitive, well-justified engineering decision, not an unimplemented placeholder.

---

### Group 4: Non-Leaf PTE V: 0->1 Eventual Visibility

**Spec Reference**:
- `norm:Svvptc_explicit_stores_update_valid_bit`: Explicitly includes **non-leaf PTEs**. Even if the leaf PTE is fully valid, as long as any non-leaf PTE along the path has V=0, translation halts at that level and triggers a page-fault; performing a V=0->1 explicit store on that non-leaf PTE is equally protected by Svvptc.

**Test Scope**: Construct a page table layout with "non-leaf PTE V=0, leaf PTE V=1 with full permissions," then store to set the non-leaf PTE V bit to 1, verifying that the eventual access succeeds. Covers two non-leaf levels: intermediate (L1) and next-to-top (L2).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVVPTC-NL-01 | L1 non-leaf V=0->1 then access leaf page | 4K leaf PTE fully valid (V=1, R=1, A=1, D=1), but its parent L1 non-leaf PTE V=0 -> store L1 PTE V=1 (no sfence) -> S-mode load the 4K leaf page | load succeeds; retry <= the limit |
| SVVPTC-NL-02 | L2 non-leaf V=0->1 then access leaf page | 4K leaf PTE fully valid, L1 non-leaf V=1, but L2 non-leaf PTE V=0 -> store L2 PTE V=1 (no sfence) -> S-mode load | load succeeds; retry <= the limit |
| SVVPTC-NL-03 | L1 non-leaf V=0->1 then fetch | 4K leaf PTE V=1, X=1, A=1; its parent L1 non-leaf V=0 -> store L1 V=1 (no sfence) -> S-mode jump to execute | fetch succeeds; retry <= the limit |

> [!NOTE]
> Construction method: First establish the leaf page and all parent non-leaf PTEs, then obtain the PTE pointer of the target non-leaf level (L1 / L2), clear V and sfence, then set V=1 (without sfence), and enter S-mode to verify.

---

### Group 5: Baseline Comparison / Sanity Check

**Spec Reference**:
- `Svvptc_reverse_boundary_baseline`: The traditional path outside Svvptc (V: 0->1 + explicit sfence) must succeed on the first access, used to eliminate false positives such as "incorrect PTE setup in the test case itself."

**Test Scope**: Form a 1:1 comparison with Group 1 / 2 / 4 -- the same PTE modification flow **with** `sfence.vma` added, verifying that without relying on Svvptc, the first access succeeds (retry count = 0).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVVPTC-BASE-01 | 4K V=0->1 + sfence immediately visible | Same setup as SVVPTC-4K-01, but immediately `sfence.vma zero, zero` after storing V=1; S-mode load | First load succeeds; retry count == 0 |
| SVVPTC-BASE-02 | 2M V=0->1 + sfence immediately visible | Same setup as SVVPTC-2M-01, with sfence; S-mode load | First load succeeds; retry count == 0 |
| SVVPTC-BASE-03 | Non-leaf V=0->1 + sfence immediately visible | Same setup as SVVPTC-NL-01, with `sfence.vma zero, zero` (rs1=x0 global flush covers non-leaf invalidation); S-mode load | First load succeeds; retry count == 0 |

> [!NOTE]
> The "retry count == 0" criterion for SVVPTC-BASE-* is a specification-independent engineering baseline, used to detect "incorrect PTE setup in the test case itself" or "retry-count statistics errors." The "retry count <= the limit" criterion for the Svvptc path cases (Groups 1~4) is the actual Svvptc compliance verification.

---

## Test Case Summary

| Group | Total Cases | Active Cases | TEST_SKIP Cases | ID Prefix | Focus |
|-------|-------------|--------------|-----------------|-----------|-------|
| Group 1 | 4 | 4 | 0 | `SVVPTC-4K-*` | 4 KiB leaf PTE, three access types + multi-round switching |
| Group 2 | 3 | 3 | 0 | `SVVPTC-2M-*` | 2 MiB megapage leaf PTE |
| Group 3 | 2 | 0 | 2 | `SVVPTC-1G-*` | 1 GiB gigapage leaf PTE (uniformly skipped due to the available physical memory constraint) |
| Group 4 | 3 | 3 | 0 | `SVVPTC-NL-*` | Non-leaf PTE (L1 / L2) |
| Group 5 | 3 | 3 | 0 | `SVVPTC-BASE-*` | Baseline comparison (with sfence) |
| **Total** | **15** | **13** | **2** | — | — |

---

## Result Determination Principles

- If the platform implements Svvptc but its behavior deviates from the SPEC (e.g., after V=0->1 the access still cannot succeed within the bounded retry limit, indicating the implementation caches Invalid PTEs without an eviction mechanism): keep the test case failing, compare against the SPEC, and record the issue in the `bugs/` directory. Modifying the test case or adding a workaround to accommodate an incorrect implementation is prohibited.
- A failure of the baseline cases (Group 5) "retry count == 0" usually points to a defect in the test case's own PTE setup or retry statistics; this should be ruled out first before judging the Svvptc path.

---

## References

- `svvptc.adoc` — Svvptc extension definition
- `supervisor.adoc` — Virtual address translation algorithm
- `svinval.adoc` — SFENCE.VMA / SINVAL.VMA semantics (reverse boundary reference)
- `Svinval_test_plan.md` — Svinval test plan (reverse scenario coverage)

---

## Appendix A: Specification Point Coverage Matrix

The table below indicates which test cases cover each specification point listed in the "Covered Specification Points" section.

| Norm ID | Covered Test IDs |
|---------|------------------|
| `norm:Svvptc_explicit_stores_update_valid_bit` | SVVPTC-4K-01~04, SVVPTC-2M-01~03, SVVPTC-NL-01~03, SVVPTC-1G-01~02 (placeholder skip) |
| `Svvptc_bounded_time_eventual_visibility` | SVVPTC-4K-01~04, SVVPTC-2M-01~03, SVVPTC-NL-01~03 |
| `Svvptc_reverse_boundary_baseline` | SVVPTC-BASE-01~03 |
