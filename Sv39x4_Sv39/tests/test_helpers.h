/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SV39_SV39X4_TEST_HELPERS_H
#define SV39_SV39X4_TEST_HELPERS_H

/* ===================================================================
 * Forward header for two-stage tests.
 *
 * 本目录（Sv39_Sv39x4）及其 8 个借用目录仅承担 VS+G 两阶段翻译
 * 测试，不包含 G-stage 独立测试。两阶段 group test 文件直接 #include
 * 本头取得：
 *   - 测试框架 (test_framework.h)
 *   - VM/Hypervisor 通用定义 (vm/vm.h, hyp/hyp_*.h)
 *   - 两阶段 helpers (hyp/two_stage*.h, hyp/test_vs_helpers.h)
 *
 * 不再提供 _setup_with_victim() / _vsfault_check() 等 G-stage 局部
 * helper（仅在原 Sv39x4/tests/ 中给 G-stage 独立测试使用）。
 *
 * 注意：与 Sv39x4 共用 kernel.ld 中的 .vm_test_region 布局，因此本
 * 头 extern 出 test_data_area / test_fault_page / test_exec_page /
 * test_exec_target 等 linker 符号。
 * =================================================================== */

#include "test_framework.h"
#include "vm/vm.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_test.h"
#include "hyp/gstage_pt.h"
#include "hyp/two_stage.h"
#include "hyp/two_stage_helpers.h"
#include "hyp/test_vs_helpers.h"

/* Linker-provided test-region symbols (see kernel.ld). */
extern uint8_t test_data_area[];
extern uint8_t test_fault_page[];
extern uint8_t test_exec_page[];
extern uint8_t test_exec_target[];
extern uint8_t __vm_test_region_start[];
extern uint8_t __vm_test_region_end[];

/* The test region base address is the start of the .vm_test_region
 * section. It is 2MB-aligned. */
#define TEST_REGION_BASE   ((uintptr_t)__vm_test_region_start)

/* Common identity-mapping flags for setting up the code/data region.
 * G-stage requires U=1 because all G-stage accesses are treated as
 * U-mode. */
#define G_FLAGS_RWXU_AD    (PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D)

#endif /* SV39_SV39X4_TEST_HELPERS_H */
