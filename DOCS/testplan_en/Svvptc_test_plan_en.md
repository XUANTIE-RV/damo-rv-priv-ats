**[中文](../testplan/Svvptc_test_plan.md) | English**

# Svvptc Extension Test Plan

This document describes the test plan for the Svvptc (Obviating Memory-Management Instructions after Marking PTEs Valid) extension. The Svvptc extension specifies that after a hart explicitly stores a leaf/non-leaf PTE with the Valid bit transitioning **from 0 to 1**, the operating system **no longer needs** to execute address translation cache synchronization instructions such as `SFENCE.VMA` / `SINVAL.VMA`; the PTE update will become visible to subsequent implicit accesses (address translations) by that hart within a **bounded timeframe**.

---

## Overview

The RISC-V Privileged Specification requires that when software modifies a PTE in memory, it must execute `SFENCE.VMA` or `SINVAL.VMA` to synchronize the hart's address translation cache (TLB / Page Walk Cache); otherwise, subsequent accesses may still use stale translations.

The Svvptc extension relaxes the synchronization requirement for **one specific direction**:

- **Covered scenario (Svvptc guarantee)**: PTE Valid bit `V: 0 → 1`, including both leaf and non-leaf PTEs. Instructions such as `SFENCE.VMA` become **redundant**; hardware guarantees that the update becomes visible to subsequent implicit accesses within a bounded timeframe, at the cost of **possibly incurring an occasional gratuitous additional page fault** (spec NOTE: "occasional gratuitous additional page fault").
- **Uncovered scenarios (sfence still required)**: Any **other** form of PTE update, including `V: 1 → 0`, permission downgrade (e.g., RW → R), PA remapping (PPN change), software clearing of A/D bits, etc. These scenarios still require `SFENCE.VMA` / `SINVAL.VMA` and are independently covered by the svinval test plan.

Possible microarchitectural implementations of Svvptc include (see `SPEC/svvptc.adoc:21-25`):
- Not implementing an address translation cache;
- Not caching Invalid PTEs;
- Using a bounded timer to automatically evict Invalid PTEs;
- Keeping the translation cache coherent with stores that modify PTEs.

This test plan focuses on the behavior of the specification in a **single hart, single thread** scenario: without invoking sfence.vma, it verifies that after a V=0→1 modification, the access **eventually** succeeds (within the bounded retry limit), and uses "immediate success with sfence" as a baseline comparison.

---

## Test Scope

### Specification Source

| File | Line/Section | Content |
|------|---------|------|
| `SPEC/svvptc.adoc` | Lines 1–28 | Svvptc extension definition; sole specification point `norm:Svvptc_explicit_stores_update_valid_bit`; NOTE explaining that the OS may omit sfence and that gratuitous page faults are tolerated |
| `SPEC/svvptc.adoc` | Lines 4–7 | Core specification: `When the Svvptc extension is implemented, explicit stores by a hart that update the Valid bit of leaf and/or non-leaf PTEs from 0 to 1 ... will eventually become visible within a bounded timeframe to subsequent implicit accesses by that hart to such PTEs.` |
| `SPEC/supervisor.adoc` | Translation algorithm (steps 1–10) | Sv39 translation flow; translation halts and raises the corresponding page-fault type when any intermediate PTE or leaf PTE has V=0 |
| `SPEC/svinval.adoc` | Full text | SFENCE.VMA / SINVAL.VMA semantics (used as boundary reference for "scenarios requiring sfence"; not within the scope of this test plan) |
| `common/vm/vm.h` | Full text | `pt_init` / `pt_map_page` / `pt_get_pte` / `vm_run_in_smode` / `vm_sfence_vma` and other VM APIs |
| `common/vm/page_table.c` | Lines 324–339 | `pt_get_pte(ctx, va, level)` actual semantics: `level` is the page table level (in Sv39: `PT_LEVEL_4K=0`/`PT_LEVEL_2M=1`/`PT_LEVEL_1G=2`), returning the PTE slot pointer corresponding to the VPN index at that level's PT page |
| `common/encoding.h` | Full text | `PTE_V` / `PTE_R` / `PTE_W` / `PTE_X` / `PTE_A` / `PTE_D` / `CAUSE_*` and other macros |
| `common/platform/{qemu_virt,spike,sail}platfrom_config.h` | — | `PLATFORM_MEM_BASE = 0x80000000`, `PLATFORM_MEM_SIZE = 0x10000000` (256 MiB), consistent across all three platforms |
| `svadu/main.c`, `svadu/Makefile`, `svadu/kernel.ld` | — | Subdirectory organization template (main.c entry point, TARGET, SPIKE_ISA_EXT, Makefile.common inclusion pattern, `__vm_test_region_start` / `__vm_test_region_2m_start` provided by each subdirectory's own `kernel.ld`) |
| `svpbmt/tests/test_nonleaf_pbmt.c` | Lines 31, 190 | Existing example: `pt_get_pte(&ctx, va, PT_LEVEL_2M)` to obtain the L1 non-leaf PTE (pointer to L0), `pt_get_pte(&ctx, va, PT_LEVEL_1G)` to obtain the L2 root non-leaf PTE (pointer to L1) |

### Covered Specification Points

| Norm ID | Original Text |
|---------|------|
| `norm:Svvptc_explicit_stores_update_valid_bit` | When the Svvptc extension is implemented, explicit stores by a hart that update the Valid bit of leaf and/or non-leaf PTEs from 0 to 1 and are visible to a hart will eventually become visible within a bounded timeframe to subsequent implicit accesses by that hart to such PTEs. |
| `"bounded time" semantics` | — |
| `Reverse boundary (baseline comparison)` | — |

### Out of Scope

- **Reverse scenarios** (V: 1→0, permission downgrade, PPN remapping, A/D software clearing): Svvptc does **not** guarantee that such updates can omit sfence; these scenarios are covered by `docs/svinval_test_plan.md` and related plans.
- **Hypervisor two-stage translation scenarios**: Svvptc behavior under VS-stage / G-stage (involves the H extension, which is not available on this test platform).
- **Sv32 / Sv48 / Sv57 modes**: Only Sv39 is covered, consistent with the existing svade/svadu/svnapot/svpbmt test plans.
- **Multi-hart coherence**: The Svvptc specification explicitly constrains only the visibility between explicit stores and implicit accesses on the **same hart**; cross-hart visibility requires mechanisms such as IPI + sfence, which are beyond the scope of Svvptc. This test framework operates in a single-core environment.
- **Composite interactions with svinval / svadu**: For example, "whether Svvptc still applies when SINVAL.VMA is used instead of SFENCE.VMA" and similar combination scenarios are covered by each extension's independent test plan and are not expanded here.
- **PMP / Smepmp and Svvptc intersection**: Related interactions are independently covered by the PMP test plan series.

---

## Design Key Points

### 1. Trap-Handler Retry Strategy (Verifying 'bounded time')

Svvptc allows "occasional gratuitous page faults," so the **first** implicit access after a V=0→1 modification may still trigger a page-fault. The test verifies "eventual visibility within bounded time" through the following approach:

1. Test code accesses the target VA in S-mode, expecting **eventual** success.
2. A custom S-mode trap handler captures the page-fault and **does not increment sepc**, instead executing `sret` directly to retry the same instruction; meanwhile, it accumulates a retry count via `sscratch`.
3. When the retry count exceeds `SVVPTC_MAX_RETRY = 1024`, the trap handler redirects sepc to an **escape label** and writes back a failure flag, which the test code reads and asserts as a failure.
4. The test case ultimately asserts: `access succeeded && retry_count <= SVVPTC_MAX_RETRY`.

> [!NOTE]
> `SVVPTC_MAX_RETRY = 1024` is a conservative initial value, based on the following rationale: Spike / Sail and other simulator implementations of Svvptc typically do not cache Invalid PTEs, so the access succeeds with 0 retries; real hardware (HAPS / RTL) requires tens to hundreds of retries. 1024 provides two orders of magnitude of headroom, and 1024 trap round-trips still complete the test in milliseconds. This upper limit may be increased during the implementation phase based on observed platform distributions (e.g., made injectable via Makefile with `-DSVVPTC_MAX_RETRY=N`), but **must not be reduced to 0** — reducing to 0 is equivalent to mandating "first-access success," which violates the spec NOTE regarding tolerance of "occasional gratuitous additional page faults."

### 2. No Explicit sfence.vma Invocation

All Group 1–4 test cases **deliberately omit** `sfence.vma`, relying on Svvptc's "bounded-time eventual visibility" guarantee. This is the core premise of the test — if an implementation incorrectly caches Invalid PTEs without an eviction mechanism, the test case will retry indefinitely until exceeding the limit and failing.

### 3. Direct Store to Modify PTE

The test obtains the PTE pointer via `pt_get_pte(&ctx, va, level)` and then uses a plain C assignment (`*pte = new_value;`) to modify the V bit, corresponding to "explicit stores by a hart" in the specification. The semantics of the `level` parameter in `pt_get_pte` are described in `common/vm/page_table.c:329`:

| `level` Value | Sv39 Meaning | Return Value |
|---|---|---|
| `PT_LEVEL_4K` (0) | Leaf-level PT page (L0) | 4K leaf PTE slot pointer |
| `PT_LEVEL_2M` (1) | L1 intermediate-level PT page | If VA uses 4K mapping → L1 non-leaf PTE (pointing to L0); if VA uses 2M superpage → L1 leaf PTE (megapage) |
| `PT_LEVEL_1G` (2) | L2 root PT page | If VA uses 4K/2M → L2 non-leaf PTE (pointing to L1); if VA uses 1G superpage → L2 leaf PTE (gigapage) |

`svpbmt/tests/test_nonleaf_pbmt.c` already provides an existing example (line 31 uses `PT_LEVEL_2M` to obtain the L1 non-leaf PTE, line 190 uses `PT_LEVEL_1G` to obtain the L2 root non-leaf PTE).

**Do not** use `pt_map_page` for remapping (the latter implies too many side effects).

> [!NOTE]
> **No** `fence rw, rw` is needed: on the same hart, a store to an address followed by a subsequent instruction read to the same address is automatically guaranteed visible by RVWMO's program order + same-address dependency. `fence rw, rw` neither replaces `sfence.vma` (it does not flush the TLB) nor provides additional semantics for PTE stores; instead, it may mislead readers into thinking it "participates in the Svvptc path." In this plan, after a PTE store, only `*pte = new_value;` is performed, with no fence added.

### 4. Instruction Pre-placement Flow for Fetch Test Cases

A leaf page with X permission and initial V=0 **cannot** have instruction bytes written to it by any store (V=0 means neither readable nor writable). Therefore, all fetch-type test cases (SVVPTC-4K-03 / SVVPTC-2M-03 / SVVPTC-NL-03) must follow this three-step pre-placement flow:

1. **Pre-placement phase**: First use `pt_map_page(va, pa, PTE_V|PTE_R|PTE_W|PTE_A|PTE_D, level)` to temporarily map the target page with RW permissions and V=1, call `init_exec_page(va)` (referencing `svadu/test_helpers.h::init_exec_page`, writing 1023 `nop` instructions + 1 `ret = 0x00008067`), then `vm_sfence_vma(0, 0)` to synchronize.
2. **Reconfiguration phase**: Use `pt_get_pte` to obtain the PTE pointer, directly store to change permissions to V=0, X=1, A=1 (removing R/W, clearing V), and `vm_sfence_vma(0, 0)` to synchronize. **This step must use sfence** — V:1→0 and permission downgrade are **not** within the Svvptc guarantee scope.
3. **Test phase**: Again use a direct store to set the V bit to 1 (**without** sfence), enter S-mode and invoke via function pointer `((void(*)(void))va)()`, using the trap-handler retry mechanism to absorb gratuitous instruction page-faults; ultimately assert `g_svvptc_retry_count <= SVVPTC_MAX_RETRY` and `!g_svvptc_escape`.

### 5. Baseline Comparison (Sanity Check)

Each core test group has a corresponding baseline in Group 5: the same PTE modification flow **with** an explicit `sfence.vma`, verifying that the "traditional path not relying on Svvptc" succeeds on the first access. This eliminates false positives such as "incorrect PTE setup in the test case itself" and increases test confidence.

### 6. Necessity of sfence in SVVPTC-4K-04 Multi-Round Switching

SVVPTC-4K-04 uses `sfence.vma` at the **end** of each round to clear the V bit **back to 0**. The sfence here is **mandatory** because:

- V: 1→0 is a PTE update **outside** the Svvptc guarantee scope; without sfence, the TLB may still hit the old V=1 translation, causing the "V=0 starting point" of the next round to not actually take effect, and the entire test degrades to "V always = 1," failing to verify the forward semantics of Svvptc.
- Only the V=0→1 transition at the **start** of each round deliberately omits sfence, corresponding to the Svvptc test objective.

### 7. Trap-Handler Retry Hook Implementation Approach

The existing `common/trap.c` architecture uses "unified M-mode handling + `trap_record.armed` single-capture," which cannot be directly reused for "S-mode multi-retry" (`trap_record.armed` is cleared after the first trigger). This test adopts the **only definitive** subdirectory-local approach, avoiding invasive changes to common:

1. In `svvptc/tests/svvptc_strap.S`, provide a local S-mode trap entry `svvptc_smode_trap_entry` (a 4-byte-aligned asm stub).
2. In `svvptc/tests/svvptc_strap.S`, simultaneously define a `.globl svvptc_escape_label` (a global symbol consisting of a single `ret` instruction) as the escape jump target.
3. The test body, running in M-mode, sets `medeleg` to delegate page-faults (cause 12 / 13 / 15) to S-mode (`vm_run_in_smode` already performs part of this delegation automatically), then calls `vm_run_in_smode` to enter S-mode; the **first** C statement in the S-mode entry function is `asm volatile("csrw stvec, %0" :: "r"(svvptc_smode_trap_entry));`, delegating stvec to the local entry.
4. Before the S-mode test function returns, restore stvec: `asm volatile("csrw stvec, %0" :: "r"(saved_stvec));` (`saved_stvec` is saved via `csrr` before step 3).
5. After exiting S-mode and returning to M-mode, `medeleg` is restored by the corresponding cleanup path of `vm_run_in_smode`.

> [!IMPORTANT]
> The alternative approach of "adding a weak hook in common/trap.c" is **not** adopted, for the following reasons: (a) it is invasive to common code and no other tests have this requirement; (b) the M-mode trap path in common and S-mode page-fault handling are conceptually two separate mechanisms, and mixing them would increase the cognitive burden of common; (c) the subdirectory-local approach is entirely sufficient and zero-invasive.

### 8. Subdirectory and Build Integration

> [!IMPORTANT]
> This planning phase **only produces `docs/svvptc_test_plan.md`**, and does not create the `svvptc/` subdirectory, modify the top-level `Makefile`, or write C test code. The subsequent implementation phase will create `svvptc/main.c`, `svvptc/Makefile` (referencing `svadu/Makefile`, with `SPIKE_ISA_EXT = _svvptc`), `svvptc/kernel.ld` (referencing `svadu/kernel.ld`, which must PROVIDE `__vm_test_region_start`, `__vm_test_region_2m_start`, `__page_tables_start/_end`, etc.), `svvptc/tests/*.c` per this document, and add `svvptc` to the top-level `Makefile`'s `EXTENSIONS` list.

---

## Test Groups

> [!IMPORTANT]
> A total of 5 test groups and 15 test cases. All tests run in Sv39 mode, in S-mode. Each group provides: Spec Reference, Test Scope, and a test case table (ID/Name/Description/Expected Result); Group 1 provides a complete C code example.

---

### Group 1: 4 KiB Leaf PTE V: 0→1 Eventual Visibility Without sfence

**Spec Reference**:
- `norm:Svvptc_explicit_stores_update_valid_bit`: Leaf PTE V: 0→1 becomes visible to subsequent implicit accesses within bounded time.

**Test Scope**: Verify on 4 KiB leaf PTEs that all three implicit access types (load / store / instruction fetch) succeed within `SVVPTC_MAX_RETRY` after V=0→1; and verify repeatability through a "multi-round V-bit switching + sfence clear back to 0" test case.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| SVVPTC-4K-01 | 4K V=0→1 load eventually visible | 4 KiB PTE initial V=0, R=1, A=1, D=1; store sets PTE.V=1 (no sfence); S-mode repeatedly loads until success | load succeeds; retry count <= MAX_RETRY |
| SVVPTC-4K-02 | 4K V=0→1 store eventually visible | 4 KiB PTE initial V=0, R=1, W=1, A=1, D=1; store sets PTE.V=1 (no sfence); S-mode repeatedly stores until success | store succeeds; retry count <= MAX_RETRY |
| SVVPTC-4K-03 | 4K V=0→1 fetch eventually visible | 4 KiB PTE initial V=0, X=1, A=1; store sets PTE.V=1 (no sfence); S-mode jumps to execute | fetch succeeds; retry count <= MAX_RETRY |
| SVVPTC-4K-04 | 4K V-bit multi-round switching visibility | Repeat 8 rounds: `sfence.vma` clears V to 0 (V:1→0 is outside Svvptc scope, **sfence required**) → sets V to 1 without sfence (V:0→1 is protected by Svvptc) → load until success; retry count accumulated independently per round | Each round succeeds; each round `retry_count <= MAX_RETRY` |

#### Key Code Example: SVVPTC-4K-01

#### S-mode Trap-Handler Retry Hook (asm Entry + C Equivalent Semantics)

Per "Design Key Point 7," this test does **not** modify `common/trap.c`, but instead provides a local S-mode trap entry in `svvptc/tests/svvptc_strap.S`, delegating via `csrw stvec, svvptc_smode_trap_entry` before entering S-mode. Below is the description of that entry's asm skeleton and equivalent C semantics.

> [!IMPORTANT]
> `svvptc_escape_label` is a **genuine global function symbol** (exported with `.globl`, terminated with `ret` in assembly), not an inline label within a test function body. This allows `la svvptc_escape_label` to resolve correctly from another translation unit. After escape, `sret` jumps directly to this stub, and `ret` returns according to the S-mode calling convention to the `vm_run_in_smode` S-mode trampoline, ultimately cleanly returning control to the M-mode test body.

Equivalent C Semantics (For Reference Only, Not Compiled):

---

### Group 2: 2 MiB Megapage Leaf PTE V: 0→1 Eventual Visibility

**Spec Reference**:
- `norm:Svvptc_explicit_stores_update_valid_bit`: The specification does not distinguish PTE levels; 2 MiB megapage leaf PTEs are equally applicable.

**Test Scope**: Verify on 2 MiB megapage leaf PTEs that all three access types (load / store / fetch) succeed within `SVVPTC_MAX_RETRY` after V=0→1.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| SVVPTC-2M-01 | 2M V=0→1 load eventually visible | 2 MiB megapage initial V=0, R=1, A=1, D=1; store V=1 (no sfence); S-mode load | load succeeds; retry <= MAX_RETRY |
| SVVPTC-2M-02 | 2M V=0→1 store eventually visible | 2 MiB megapage initial V=0, R=1, W=1, A=1, D=1; store V=1 (no sfence); S-mode store | store succeeds; retry <= MAX_RETRY |
| SVVPTC-2M-03 | 2M V=0→1 fetch eventually visible | 2 MiB megapage initial V=0, X=1, A=1; store V=1 (no sfence); S-mode jump to execute | fetch succeeds; retry <= MAX_RETRY |

> [!NOTE]
> The 2 MiB test region uses `__vm_test_region_2m_start` (referencing `svadu/test_helpers.h`). `pt_get_pte(&ctx, va, PT_LEVEL_2M)` returns the L1-level PTE pointer, modified directly via `pte_set_valid` store.

---

### Group 3: 1 GiB Gigapage Leaf PTE V: 0→1 Eventual Visibility

**Spec Reference**:
- `norm:Svvptc_explicit_stores_update_valid_bit`: 1 GiB gigapage leaf PTEs are equally applicable.

**Test Scope**: Verify on 1 GiB gigapage leaf PTEs that load / store succeed within `SVVPTC_MAX_RETRY` after V=0→1. Fetch is not separately covered for 1G test cases (equivalent to 2M, providing no additional coverage value).

> [!IMPORTANT]
> **Infeasibility analysis**: To verify Svvptc on a 1 GiB gigapage leaf PTE, the entire 1 GiB virtual address range starting at `0x80000000` must be covered by a **single standalone 1G leaf PTE**, with the V bit cleared to 0, causing translation to follow that leaf PTE's V=0 path. However:
>
> 1. The code segment, stack, `__page_tables_*` pool, `__smode_stack_*`, and UART MMIO required for the test to run are all placed within the `[0x80000000, 0x80000000 + 256 MiB)` range via `kernel.ld` (referencing `svadu/kernel.ld`) (`PLATFORM_MEM_SIZE = 0x10000000`).
> 2. Once that 1G leaf PTE covers the entire `[0x80000000, 0x80000000 + 1 GiB)` virtual address range, **all** of the above regions that must remain accessible would be controlled by the same PTE, making it impossible to continue fetching instructions / accessing the stack / accessing page tables during the V=0 period.
> 3. The presence of a leaf-at-L2 PTE in the Sv39 translation process causes any 4K / 2M PTEs within that 1G subspace to be **shadowed** (once a leaf PTE matches, translation terminates), so 4K / 2M PTEs cannot be used to "bridge" the code segment.
> 4. The only clean solution would be to relocate code/stack/page tables/MMIO to physical memory outside `[0x80000000 + 1 GiB, ...]`, but the physical memory upper bound on all three current platforms is `0x80000000 + 256 MiB`, with no available relocation target address.
>
> Conclusion: **SVVPTC-1G-* cannot be cleanly implemented via pure software on the three current platforms (qemu_virt / spike / sail) with `PLATFORM_MEM_SIZE = 256 MiB`.**

> [!IMPORTANT]
> Therefore, this plan **determines**: both SVVPTC-1G-* test cases uniformly use `TEST_SKIP` implementation, printing a clear skip reason (`"1G gigapage test skipped: needs PLATFORM_MEM_SIZE > 1 GiB to host code/stack outside the gigapage region; current = 256 MiB"`). Specification coverage is jointly fulfilled by Group 1 (4K) + Group 2 (2M) + Group 4 (non-leaf); 1G gigapage and 2M megapage are **completely equivalent** in Svvptc semantics (both are leaf-PTE V:0→1, and the specification text `leaf and/or non-leaf PTEs from 0 to 1` does not distinguish the specific level of the leaf), constituting no gap in specification coverage.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| SVVPTC-1G-01 | 1G V=0→1 load placeholder (skip) | Function body contains only `TEST_SKIP("1G gigapage test skipped: needs PLATFORM_MEM_SIZE > 1 GiB ...")` | `[SKIP]` output, not counted as failure |
| SVVPTC-1G-02 | 1G V=0→1 store placeholder (skip) | Same as SVVPTC-1G-01 | `[SKIP]` output, not counted as failure |

> [!NOTE]
> The purpose of retaining both SVVPTC-1G-01 / -02 `TEST_SKIP` test cases: to explicitly document in the test report that "the 1G dimension is deliberately skipped and the reason why," preventing future maintainers from mistakenly assuming it was an oversight. This "`TEST_SKIP` + explicit reason" pattern is the standard usage of the `TEST_SKIP` macro in `common/test_framework.h:99`, fully consistent with existing skip cases such as `svnapot/tests/test_sv_modes.c:49` ("Sv48 not supported on this platform") and `svadu/tests/test_csr_adue.c:32` ("Platform does not implement Svadu") — **not an unimplemented placeholder, but a definitive, well-justified engineering decision**.

---

### Group 4: Non-Leaf PTE V: 0→1 Eventual Visibility

**Spec Reference**:
- `norm:Svvptc_explicit_stores_update_valid_bit`: Explicitly includes **non-leaf PTEs**. Even if the leaf PTE is fully valid, as long as any non-leaf PTE along the path has V=0, translation halts at that level and triggers a page-fault; performing a V=0→1 explicit store on that non-leaf PTE is equally protected by Svvptc.

**Test Scope**: Construct a page table layout with "non-leaf PTE V=0, leaf PTE V=1 with full permissions," then store to set the non-leaf PTE V bit to 1, verifying that the eventual access succeeds. Covers two non-leaf levels: intermediate (L1) and next-to-top (L2).

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| SVVPTC-NL-01 | L1 non-leaf V=0→1 then access leaf page | 4K leaf PTE fully valid (V=1, R=1, A=1, D=1), but its parent L1 non-leaf PTE V=0 → store L1 PTE V=1 (no sfence) → S-mode load 4K leaf page | load succeeds; retry <= MAX_RETRY |
| SVVPTC-NL-02 | L2 non-leaf V=0→1 then access leaf page | 4K leaf PTE fully valid, L1 non-leaf V=1, but L2 non-leaf PTE V=0 → store L2 PTE V=1 (no sfence) → S-mode load | load succeeds; retry <= MAX_RETRY |
| SVVPTC-NL-03 | L1 non-leaf V=0→1 then fetch | 4K leaf PTE V=1, X=1, A=1; its parent L1 non-leaf V=0 → store L1 V=1 (no sfence) → S-mode jump to execute | fetch succeeds; retry <= MAX_RETRY |

> [!NOTE]
> Construction method: First use `pt_map_page(va, pa, PTE_V|PTE_R|PTE_A|PTE_D, PT_LEVEL_4K)` to establish the leaf page and all parent non-leaf PTEs, then use `pt_get_pte(&ctx, va, PT_LEVEL_2M)` (to obtain the L1 non-leaf PTE) or `pt_get_pte(&ctx, va, PT_LEVEL_1G)` (to obtain the L2 non-leaf PTE) to get the non-leaf PTE pointer, use `pte_set_valid(pte, 0)` to clear V, sfence, then `pte_set_valid(pte, 1)` (without sfence), enter S-mode to verify.

---

### Group 5: Baseline Comparison / Sanity Check

**Spec Reference**:
- Reverse boundary: The traditional path outside Svvptc (V: 0→1 + explicit sfence) must succeed on the first access, used to eliminate false positives such as "incorrect PTE setup in the test case itself."

**Test Scope**: Form a 1:1 comparison with Group 1 / 2 / 4 — the same PTE modification flow **with** `sfence.vma` added, verifying that without relying on Svvptc, the first access succeeds (retry count = 0).

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| SVVPTC-BASE-01 | 4K V=0→1 + sfence immediately visible | Same setup as SVVPTC-4K-01, but immediately `sfence.vma zero, zero` after storing V=1; S-mode load | First load succeeds; `retry_count == 0` |
| SVVPTC-BASE-02 | 2M V=0→1 + sfence immediately visible | Same setup as SVVPTC-2M-01, with sfence; S-mode load | First load succeeds; `retry_count == 0` |
| SVVPTC-BASE-03 | Non-leaf V=0→1 + sfence immediately visible | Same setup as SVVPTC-NL-01, with `sfence.vma zero, zero` (rs1=x0 global flush covers non-leaf invalidation); S-mode load | First load succeeds; `retry_count == 0` |

> [!NOTE]
> The "`retry_count == 0`" criterion for SVVPTC-BASE-* is a specification-independent engineering baseline, used to detect "incorrect PTE setup in the test case itself" or "trap-handler hook counting errors." The "`retry_count <= MAX_RETRY`" criterion for Svvptc path test cases (Group 1–4) is the actual Svvptc compliance verification.

---

## Test Case Summary

| Group | Total Cases | Active Cases | TEST_SKIP Cases | ID Prefix | Focus |
|-------|---------|---------|---------------|---------|--------|
| Group 1 | 4 | 4 | 0 | `SVVPTC-4K-*` | 4 KiB leaf PTE, three access types + multi-round switching |
| Group 2 | 3 | 3 | 0 | `SVVPTC-2M-*` | 2 MiB megapage leaf PTE |
| Group 3 | 2 | 0 | 2 | `SVVPTC-1G-*` | 1 GiB gigapage leaf PTE (uniformly skipped due to 256 MiB physical memory constraint) |
| Group 4 | 3 | 3 | 0 | `SVVPTC-NL-*` | Non-leaf PTE (L1 / L2) |
| Group 5 | 3 | 3 | 0 | `SVVPTC-BASE-*` | Baseline comparison (with sfence) |
| **Total** | **15** | **13** | **2** | — | — |

---

## Implementation Phase Recommendations (Out of Scope for This Plan)

> [!IMPORTANT]
> The following content is for reference only in subsequent implementation phases, and **is not executed during this plan document generation phase**. This document only produces `docs/svvptc_test_plan.md`, and does not create the `svvptc/` subdirectory, modify the top-level `Makefile`, or write C test code.

1. Create the `svvptc/` subdirectory, referencing the `svadu/` structure:
   - `main.c`: test entry point (referencing `svadu/main.c`, but **no need** to save `g_menvcfg_reset_value`, as Svvptc has no CSR control bit);
   - `kernel.ld`: linker script (referencing `svadu/kernel.ld`, must PROVIDE `__vm_test_region_start`, `__vm_test_region_2m_start`, `__page_tables_start`, `__page_tables_end`, `__smode_stack_start`, `__smode_stack_end`, and all other symbols required by common modules);
   - `Makefile`: `TARGET = svvptc_test.elf`, `ENABLE_VM = 1`, `SPIKE_ISA_EXT = _svvptc`, `include ../common/Makefile.common`;
   - `tests/`: `test_helpers.h`, `test_register.c`, `test_4k_leaf.c`, `test_2m_megapage.c`, `test_1g_gigapage.c`, `test_non_leaf.c`, `test_baseline.c`, and a local S-mode trap entry asm file (e.g., `tests/svvptc_strap.S`).
2. **Trap-handler retry hook**: Per "Design Key Point 7," adopt the subdirectory-local approach: implement a 4-byte-aligned S-mode trap entry in `svvptc/tests/svvptc_strap.S`, delegating page-faults via `csrw stvec, svvptc_smode_trap_entry` before entering S-mode; no dependency on modifying `common/trap.c`.
3. In the top-level `Makefile`'s `EXTENSIONS` list (currently `pmp smepmp spmp pmp_sv39 pmp_sv48 pmp_sv57 sv39 sv48 sv57 sv39x4 svnapot svinval aia svadu`), append `svvptc` at the end.
4. Verification:
   - `make spike-svvptc`: In the `Makefile`, `SPIKE_ISA_EXT = _svvptc` appends `_svvptc` to the `--isa=` string. Regardless of whether the current Spike version recognizes `_svvptc` as a valid ISA string, the semantics of this test do not depend on Spike explicitly implementing Svvptc — Spike's behavior of not caching Invalid PTEs naturally satisfies the Svvptc specification, and the `retry_count` for all Group 1 / 2 / 4 test cases on Spike is expected to be 0, with Group 5 baseline test cases also at 0; matching output from both constitutes compliance. If Spike fails to start due to not recognizing the `_svvptc` string, change to `SPIKE_ISA_EXT =` (empty), and print a line in the main.c banner: "`Svvptc: relying on Spike's intrinsic 'no Invalid-PTE caching' behavior`".
   - `make sail-svvptc`: Same handling logic as spike.
   - Real hardware / HAPS: Observe the `retry_count` distribution; if all test cases have `retry_count < SVVPTC_MAX_RETRY` and `g_svvptc_escape == false`, it is considered compliant; if any test case reaches escape (`retry_count == SVVPTC_MAX_RETRY` and `g_svvptc_escape == true`), submit a hardware defect report.
