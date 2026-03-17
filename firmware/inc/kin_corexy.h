/* kin_corexy.h - CoreXY kinematics implementation (A/B belts for X/Y)
 *
 * Convention used here:
 *  - Cartesian axes: X, Y, Z (mm)
 *  - Joint axes:    J0=A, J1=B, J2=Z, J3=AUX (optional)
 *
 * Standard CoreXY mapping (common):
 *  - A = X + Y
 *  - B = X - Y
 *
 * Inverse:
 *  - X = (A + B)/2
 *  - Y = (A - B)/2
 *
 * You can flip signs with invert flags if your motors are wired opposite.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "kinematics.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* steps per mm for each JOINT (motor) axis: A, B, Z, AUX */
    float steps_per_mm[KIN_MAX_JOINT_AXES];

    /* Optional joint inversion (true => negate that joint coordinate) */
    bool invert_joint[KIN_MAX_JOINT_AXES];

    /* Optional cart inversion (true => negate that cart coordinate) */
    bool invert_cart[KIN_MAX_CART_AXES];

    /* Segmentation:
     * - CoreXY usually doesn't require segmentation (it is linear),
     *   but leaving it here lets you clamp max segment length.
     * - 0 disables segmentation.
     */
    float max_segment_len_mm;

    /* Homing tuning (simple) */
    float home_fast_mm_min;
    float home_slow_mm_min;
} kin_corexy_cfg_t;

/* Install CoreXY implementation into global g_kin and keep config internally. */
void kin_corexy_install(const kin_corexy_cfg_t *cfg);

/* Optional: update config at runtime (e.g., after $$ settings change). */
void kin_corexy_set_cfg(const kin_corexy_cfg_t *cfg);
void kin_corexy_get_cfg(kin_corexy_cfg_t *out);

#ifdef __cplusplus
}
#endif