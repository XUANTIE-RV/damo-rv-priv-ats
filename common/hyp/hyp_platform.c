/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * common/hyp/hyp_platform.c — Platform capability probing
 * =================================================================== */

#include "hyp_platform.h"
#include "hyp_csr.h"
#include "hyp_vs_trap.h"

/* Singleton cached capability structure */
static platform_caps_t _caps;

const platform_caps_t *platform_probe(void) {
    if (_caps.probed)
        return &_caps;

    /* ----- satp mode support ----- */
    _caps.satp_sv39 = satp_supports_mode(SATP_MODE_SV39);
    _caps.satp_sv48 = satp_supports_mode(SATP_MODE_SV48);
    _caps.satp_sv57 = satp_supports_mode(SATP_MODE_SV57);

    /* ----- vsatp mode support ----- */
    _caps.vsatp_sv39 = vsatp_supports_mode(SATP_MODE_SV39);
    _caps.vsatp_sv48 = vsatp_supports_mode(SATP_MODE_SV48);
    _caps.vsatp_sv57 = vsatp_supports_mode(SATP_MODE_SV57);

    /* ----- hgatp mode support ----- */
    _caps.hgatp_sv39x4 = hgatp_supports_mode(HGATP_MODE_SV39X4);
    _caps.hgatp_sv48x4 = hgatp_supports_mode(HGATP_MODE_SV48X4);
    _caps.hgatp_sv57x4 = hgatp_supports_mode(HGATP_MODE_SV57X4);

    /* ----- ASID / VMID widths ----- */
    _caps.satp_asid_bits = satp_asid_width();
    _caps.vsatp_asid_bits = vsatp_asid_width();
    _caps.hgatp_vmid_bits = hgatp_vmid_width();

    /* ----- Sub-extension availability ----- */
    _caps.ssstateen = stateen_is_available();
    _caps.vstvec_vectored = vstvec_supports_vectored();

    /* ----- hcounteren writability (Shcounterenw) -----
     * Per Shcounterenw spec: hcounteren must be writable (not hardwired 0).
     * Probe by writing all-ones and checking non-zero readback. */
    {
        uintptr_t saved = hcounteren_read();
        hcounteren_write(~(uintptr_t)0);
        uintptr_t rb = hcounteren_read();
        hcounteren_write(saved);
        _caps.shcounterenw = (rb != 0);
    }

    /* ----- Counter implementation bitmap ----- */
    _caps.counters_implemented = counteren_probe_implemented();

    _caps.probed = true;
    return &_caps;
}

const platform_caps_t *platform_caps_get(void) {
    if (!_caps.probed)
        return (const platform_caps_t *)0;
    return &_caps;
}
