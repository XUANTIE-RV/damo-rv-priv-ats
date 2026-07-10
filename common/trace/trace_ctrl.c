/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Trace control MMIO register access implementation.
 */

#include "trace_ctrl.h"

/* Base addresses (initialized by trace_ctrl_init) */
static uintptr_t g_encoder_base = 0;
static uintptr_t g_ramsink_base = 0;

/* ===================================================================
 * MMIO access helpers
 * =================================================================== */
static inline uint32_t mmio_read32(uintptr_t addr)
{
    return *(volatile uint32_t *)addr;
}

static inline void mmio_write32(uintptr_t addr, uint32_t val)
{
    *(volatile uint32_t *)addr = val;
}

/* ===================================================================
 * Initialization
 * =================================================================== */
void trace_ctrl_init(uintptr_t encoder_base, uintptr_t ramsink_base)
{
    g_encoder_base = encoder_base;
    g_ramsink_base = ramsink_base;
}

/* ===================================================================
 * Detection
 * =================================================================== */
bool trace_detect(void)
{
    if (g_encoder_base == 0)
        return false;

    /* Read trTeImpl and check compType == ENCODER */
    uint32_t impl = mmio_read32(g_encoder_base + TRTE_IMPL_OFFSET);
    uint8_t comp_type = (impl & TRTE_COMP_TYPE_MASK) >> TRTE_COMP_TYPE_SHIFT;

    return (comp_type == TRTE_COMP_TYPE_ENCODER);
}

/* ===================================================================
 * Activate/Deactivate
 * =================================================================== */
void trace_activate(void)
{
    if (g_encoder_base == 0)
        return;

    uint32_t ctrl = mmio_read32(g_encoder_base + TRTE_CONTROL_OFFSET);
    ctrl |= TRTE_ACTIVE_BIT;
    mmio_write32(g_encoder_base + TRTE_CONTROL_OFFSET, ctrl);
}

void trace_deactivate(void)
{
    if (g_encoder_base == 0)
        return;

    uint32_t ctrl = mmio_read32(g_encoder_base + TRTE_CONTROL_OFFSET);
    ctrl &= ~TRTE_ACTIVE_BIT;
    mmio_write32(g_encoder_base + TRTE_CONTROL_OFFSET, ctrl);
}

bool trace_is_active(void)
{
    if (g_encoder_base == 0)
        return false;

    uint32_t ctrl = mmio_read32(g_encoder_base + TRTE_CONTROL_OFFSET);
    return (ctrl & TRTE_ACTIVE_BIT) != 0;
}

/* ===================================================================
 * Enable/Disable
 * =================================================================== */
void trace_enable(void)
{
    if (g_encoder_base == 0)
        return;

    uint32_t ctrl = mmio_read32(g_encoder_base + TRTE_CONTROL_OFFSET);
    ctrl |= TRTE_ENABLE_BIT;
    mmio_write32(g_encoder_base + TRTE_CONTROL_OFFSET, ctrl);
}

void trace_disable(void)
{
    if (g_encoder_base == 0)
        return;

    uint32_t ctrl = mmio_read32(g_encoder_base + TRTE_CONTROL_OFFSET);
    ctrl &= ~TRTE_ENABLE_BIT;
    mmio_write32(g_encoder_base + TRTE_CONTROL_OFFSET, ctrl);
}

bool trace_is_enabled(void)
{
    if (g_encoder_base == 0)
        return false;

    uint32_t ctrl = mmio_read32(g_encoder_base + TRTE_CONTROL_OFFSET);
    return (ctrl & TRTE_ENABLE_BIT) != 0;
}

/* ===================================================================
 * Mode control
 * =================================================================== */
bool trace_set_mode(uint8_t mode)
{
    if (g_encoder_base == 0)
        return false;

    uint32_t ctrl = mmio_read32(g_encoder_base + TRTE_CONTROL_OFFSET);
    ctrl &= ~TRTE_INST_MODE_MASK;
    ctrl |= ((uint32_t)mode << TRTE_INST_MODE_SHIFT) & TRTE_INST_MODE_MASK;
    mmio_write32(g_encoder_base + TRTE_CONTROL_OFFSET, ctrl);

    /* Verify write succeeded */
    uint32_t readback = mmio_read32(g_encoder_base + TRTE_CONTROL_OFFSET);
    uint8_t readback_mode = (readback & TRTE_INST_MODE_MASK) >> TRTE_INST_MODE_SHIFT;

    return (readback_mode == mode);
}

uint8_t trace_get_mode(void)
{
    if (g_encoder_base == 0)
        return TRTE_INST_MODE_DISABLED;

    uint32_t ctrl = mmio_read32(g_encoder_base + TRTE_CONTROL_OFFSET);
    return (ctrl & TRTE_INST_MODE_MASK) >> TRTE_INST_MODE_SHIFT;
}

/* ===================================================================
 * Full register access
 * =================================================================== */
uint32_t trace_read_control(void)
{
    if (g_encoder_base == 0)
        return 0;

    return mmio_read32(g_encoder_base + TRTE_CONTROL_OFFSET);
}

void trace_write_control(uint32_t val)
{
    if (g_encoder_base == 0)
        return;

    mmio_write32(g_encoder_base + TRTE_CONTROL_OFFSET, val);
}

/* ===================================================================
 * Implementation registers
 * =================================================================== */
uint32_t trace_read_impl(void)
{
    if (g_encoder_base == 0)
        return 0;

    return mmio_read32(g_encoder_base + TRTE_IMPL_OFFSET);
}

uint32_t trace_read_features(void)
{
    if (g_encoder_base == 0)
        return 0;

    return mmio_read32(g_encoder_base + TRTE_INST_FEATURES_OFFSET);
}

uint32_t trace_read_timestamp_ctrl(void)
{
    if (g_encoder_base == 0)
        return 0;

    return mmio_read32(g_encoder_base + TRTE_TIMESTAMP_CTRL_OFFSET);
}

/* ===================================================================
 * Queue status
 * =================================================================== */
bool trace_is_empty(void)
{
    if (g_encoder_base == 0)
        return true;

    uint32_t ctrl = mmio_read32(g_encoder_base + TRTE_CONTROL_OFFSET);
    return (ctrl & TRTE_EMPTY_BIT) != 0;
}

/* ===================================================================
 * Configuration helpers
 * =================================================================== */
void trace_set_sync_config(uint8_t mode, uint8_t max_interval)
{
    if (g_encoder_base == 0)
        return;

    uint32_t ctrl = mmio_read32(g_encoder_base + TRTE_CONTROL_OFFSET);
    ctrl &= ~(TRTE_INST_SYNC_MODE_MASK | TRTE_INST_SYNC_MAX_MASK);
    ctrl |= ((uint32_t)mode << TRTE_INST_SYNC_MODE_SHIFT) & TRTE_INST_SYNC_MODE_MASK;
    ctrl |= ((uint32_t)max_interval << TRTE_INST_SYNC_MAX_SHIFT) & TRTE_INST_SYNC_MAX_MASK;
    mmio_write32(g_encoder_base + TRTE_CONTROL_OFFSET, ctrl);
}

void trace_set_context_enable(bool enable)
{
    if (g_encoder_base == 0)
        return;

    uint32_t ctrl = mmio_read32(g_encoder_base + TRTE_CONTROL_OFFSET);
    if (enable)
        ctrl |= TRTE_CONTEXT_BIT;
    else
        ctrl &= ~TRTE_CONTEXT_BIT;
    mmio_write32(g_encoder_base + TRTE_CONTROL_OFFSET, ctrl);
}

void trace_set_inhibit_src(bool inhibit)
{
    if (g_encoder_base == 0)
        return;

    uint32_t ctrl = mmio_read32(g_encoder_base + TRTE_CONTROL_OFFSET);
    if (inhibit)
        ctrl |= TRTE_INHIBIT_SRC_BIT;
    else
        ctrl &= ~TRTE_INHIBIT_SRC_BIT;
    mmio_write32(g_encoder_base + TRTE_CONTROL_OFFSET, ctrl);
}

void trace_set_stall_enable(bool enable)
{
    if (g_encoder_base == 0)
        return;

    uint32_t ctrl = mmio_read32(g_encoder_base + TRTE_CONTROL_OFFSET);
    if (enable)
        ctrl |= TRTE_INST_STALL_ENA_BIT;
    else
        ctrl &= ~TRTE_INST_STALL_ENA_BIT;
    mmio_write32(g_encoder_base + TRTE_CONTROL_OFFSET, ctrl);
}

void trace_set_timestamp_enable(bool enable)
{
    if (g_encoder_base == 0)
        return;

    uint32_t tsc = mmio_read32(g_encoder_base + TRTE_TIMESTAMP_CTRL_OFFSET);
    if (enable)
        tsc |= TRTS_ENABLE_BIT;
    else
        tsc &= ~TRTS_ENABLE_BIT;
    mmio_write32(g_encoder_base + TRTE_TIMESTAMP_CTRL_OFFSET, tsc);
}

/* ===================================================================
 * Feature control
 * =================================================================== */
void trace_set_sequential_jump(bool enable)
{
    if (g_encoder_base == 0)
        return;

    uint32_t feat = mmio_read32(g_encoder_base + TRTE_INST_FEATURES_OFFSET);
    if (enable)
        feat |= TRTE_INST_EN_SEQ_JUMP_BIT;
    else
        feat &= ~TRTE_INST_EN_SEQ_JUMP_BIT;
    mmio_write32(g_encoder_base + TRTE_INST_FEATURES_OFFSET, feat);
}

void trace_set_implicit_return_mode(uint8_t mode)
{
    if (g_encoder_base == 0)
        return;

    uint32_t feat = mmio_read32(g_encoder_base + TRTE_INST_FEATURES_OFFSET);
    feat &= ~(TRTE_INST_IMPLICIT_RET_MODE_MASK | TRTE_INST_EN_IMPLICIT_RET_BIT);
    if (mode != TRTE_IMPLICIT_RET_OFF)
    {
        feat |= TRTE_INST_EN_IMPLICIT_RET_BIT;
        feat |= ((uint32_t)mode << TRTE_INST_IMPLICIT_RET_MODE_SHIFT) & TRTE_INST_IMPLICIT_RET_MODE_MASK;
    }
    mmio_write32(g_encoder_base + TRTE_INST_FEATURES_OFFSET, feat);
}

void trace_set_repeated_history(bool enable)
{
    if (g_encoder_base == 0)
        return;

    uint32_t feat = mmio_read32(g_encoder_base + TRTE_INST_FEATURES_OFFSET);
    if (enable)
        feat |= TRTE_INST_EN_REPEAT_HIST_BIT;
    else
        feat &= ~TRTE_INST_EN_REPEAT_HIST_BIT;
    mmio_write32(g_encoder_base + TRTE_INST_FEATURES_OFFSET, feat);
}

void trace_set_all_jumps(bool enable)
{
    if (g_encoder_base == 0)
        return;

    uint32_t feat = mmio_read32(g_encoder_base + TRTE_INST_FEATURES_OFFSET);
    if (enable)
        feat |= TRTE_INST_EN_ALL_JUMPS_BIT;
    else
        feat &= ~TRTE_INST_EN_ALL_JUMPS_BIT;
    mmio_write32(g_encoder_base + TRTE_INST_FEATURES_OFFSET, feat);
}

void trace_set_extend_addr_msb(bool enable)
{
    if (g_encoder_base == 0)
        return;

    uint32_t feat = mmio_read32(g_encoder_base + TRTE_INST_FEATURES_OFFSET);
    if (enable)
        feat |= TRTE_INST_EXTEND_ADDR_MSB_BIT;
    else
        feat &= ~TRTE_INST_EXTEND_ADDR_MSB_BIT;
    mmio_write32(g_encoder_base + TRTE_INST_FEATURES_OFFSET, feat);
}

void trace_set_src_id(uint32_t id)
{
    if (g_encoder_base == 0)
        return;

    uint32_t feat = mmio_read32(g_encoder_base + TRTE_INST_FEATURES_OFFSET);
    feat &= ~TRTE_SRC_ID_MASK;
    feat |= (id << TRTE_SRC_ID_SHIFT) & TRTE_SRC_ID_MASK;
    mmio_write32(g_encoder_base + TRTE_INST_FEATURES_OFFSET, feat);
}

uint32_t trace_get_src_id(void)
{
    if (g_encoder_base == 0)
        return 0;

    uint32_t feat = mmio_read32(g_encoder_base + TRTE_INST_FEATURES_OFFSET);
    return (feat & TRTE_SRC_ID_MASK) >> TRTE_SRC_ID_SHIFT;
}

uint8_t trace_get_src_bits(void)
{
    if (g_encoder_base == 0)
        return 0;

    uint32_t feat = mmio_read32(g_encoder_base + TRTE_INST_FEATURES_OFFSET);
    return (feat & TRTE_SRC_BITS_MASK) >> TRTE_SRC_BITS_SHIFT;
}

/* ===================================================================
 * RAM Sink API
 * =================================================================== */
bool trace_ram_detect(void)
{
    if (g_ramsink_base == 0)
        return false;

    /* Try to read control register - should not return all 1s if present */
    uint32_t ctrl = mmio_read32(g_ramsink_base + TRRAM_CONTROL_OFFSET);
    return (ctrl != 0xFFFFFFFF);
}

void trace_ram_configure(uint64_t start, uint64_t limit)
{
    if (g_ramsink_base == 0)
        return;

    mmio_write32(g_ramsink_base + TRRAM_START_LOW_OFFSET, (uint32_t)start);
    mmio_write32(g_ramsink_base + TRRAM_START_HIGH_OFFSET, (uint32_t)(start >> 32));
    mmio_write32(g_ramsink_base + TRRAM_LIMIT_LOW_OFFSET, (uint32_t)limit);
    mmio_write32(g_ramsink_base + TRRAM_LIMIT_HIGH_OFFSET, (uint32_t)(limit >> 32));
}

uint64_t trace_ram_get_wp(void)
{
    if (g_ramsink_base == 0)
        return 0;

    uint32_t wp_low = mmio_read32(g_ramsink_base + TRRAM_WP_LOW_OFFSET);
    uint32_t wp_high = mmio_read32(g_ramsink_base + TRRAM_WP_HIGH_OFFSET);
    return ((uint64_t)wp_high << 32) | wp_low;
}

uint64_t trace_ram_get_rp(void)
{
    if (g_ramsink_base == 0)
        return 0;

    uint32_t rp_low = mmio_read32(g_ramsink_base + TRRAM_RP_LOW_OFFSET);
    uint32_t rp_high = mmio_read32(g_ramsink_base + TRRAM_RP_HIGH_OFFSET);
    return ((uint64_t)rp_high << 32) | rp_low;
}

void trace_ram_set_rp(uint64_t rp)
{
    if (g_ramsink_base == 0)
        return;

    mmio_write32(g_ramsink_base + TRRAM_RP_LOW_OFFSET, (uint32_t)rp);
    mmio_write32(g_ramsink_base + TRRAM_RP_HIGH_OFFSET, (uint32_t)(rp >> 32));
}

size_t trace_ram_read_bytes(uint8_t *buf, size_t maxlen)
{
    if (g_ramsink_base == 0)
        return 0;

    uint64_t rp = trace_ram_get_rp();
    uint64_t wp = trace_ram_get_wp();

    if (rp >= wp)
        return 0;

    size_t avail = (size_t)(wp - rp);
    size_t to_read = (avail < maxlen) ? avail : maxlen;

    /* Read byte-by-byte from RAM sink data register */
    for (size_t i = 0; i < to_read; i++)
    {
        uint32_t data = mmio_read32(g_ramsink_base + TRRAM_DATA_OFFSET);
        buf[i] = (uint8_t)(data & 0xFF);
    }

    /* Update read pointer */
    trace_ram_set_rp(rp + to_read);

    return to_read;
}

void trace_ram_reset(void)
{
    if (g_ramsink_base == 0)
        return;

    /* Reset read pointer to start */
    uint32_t start_low = mmio_read32(g_ramsink_base + TRRAM_START_LOW_OFFSET);
    uint32_t start_high = mmio_read32(g_ramsink_base + TRRAM_START_HIGH_OFFSET);
    uint64_t start = ((uint64_t)start_high << 32) | start_low;

    trace_ram_set_rp(start);
}
