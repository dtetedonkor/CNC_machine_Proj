/* kinematics.h - Core kinematics API (machine units <-> joint/steps <-> Cartesian)
 *
 * Purpose:
 *  - Hide motion geometry (cartesian / coreXY / delta / etc.) behind a small vtable.
 *  - Planner & protocol operate in "Cartesian/mm" space.
 *  - Stepper layer consumes "joint/step" space.
 *
 * Notes:
 *  - "joint" means motor/joint axes (could match X/Y/Z for Cartesian, or be A/B/C for CoreXY, etc.)
 *  - This header intentionally avoids tying to a specific planner struct; you can adapt types later.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef KIN_MAX_CART_AXES
#define KIN_MAX_CART_AXES 3u   /* X,Y,Z in Cartesian space */
#endif

#ifndef KIN_MAX_JOINT_AXES
#define KIN_MAX_JOINT_AXES 4u  /* motors/joints; e.g., CoreXY uses 2 for XY, plus Z/A */
#endif

typedef struct { float v[KIN_MAX_CART_AXES]; } kin_cart_t;   /* mm or user units */
typedef struct { float v[KIN_MAX_JOINT_AXES]; } kin_joint_t; /* joint-space mm-equivalent */
typedef struct { int32_t v[KIN_MAX_JOINT_AXES]; } kin_steps_t;

typedef struct {
    /* optional: feed/accel/junction limits you want the kinematics to know about */
    float feed_mm_min;
    float accel_mm_s2;
    float junction_dev_mm;
} kin_motion_hint_t;

/* Homing / limit-related modes (keep tiny; expand later) */
typedef enum {
    KIN_HOME_FAST = 0,
    KIN_HOME_SLOW = 1
} kin_home_mode_t;

/* Bitmask of Cartesian axes (bit0=X, bit1=Y, bit2=Z, ...) */
typedef uint32_t kin_axis_mask_t;

/* ----------------------------- Kinematics vtable ----------------------------- */

typedef struct kin_iface {
    /* Metadata */
    uint8_t cart_axes;   /* typically 3 */
    uint8_t joint_axes;  /* motors/joints in use */

    /* Convert motor steps -> machine Cartesian position.
     * Used for status reports / sync after homing / etc.
     */
    void (*steps_to_cart)(const kin_steps_t *steps, kin_cart_t *out_cart);

    /* Convert Cartesian position -> joint coordinates (mm-equivalent).
     * Used to build a motion block before step generation.
     */
    bool (*cart_to_joint)(const kin_cart_t *cart, kin_joint_t *out_joint);

    /* Convert joint coordinates -> Cartesian (optional but handy for reporting/debug). */
    bool (*joint_to_cart)(const kin_joint_t *joint, kin_cart_t *out_cart);

    /* Segmenting hook:
     * - Some kinematics (delta, SCARA, CoreXY with constraints) may need to subdivide
     *   a straight Cartesian move into smaller segments in joint space.
     * - Return true when a segment was produced in out_cart_next.
     * - Return false when segmentation is finished (no more segments).
     *
     * Typical usage:
     *   init=true on first call for a move; subsequent calls init=false until false returned.
     */
    bool (*segment_move)(const kin_cart_t *cart_target,
                         const kin_cart_t *cart_current,
                         const kin_motion_hint_t *hint,
                         bool init,
                         kin_cart_t *out_cart_next);

    /* Limits / homing support:
     * Given a limit switch index (platform-specific), return which Cartesian axis it maps to.
     * Example: idx=0 -> X min, idx=1 -> X max, etc. Your HAL can define the ordering.
     */
    kin_axis_mask_t (*limit_index_to_axes)(uint8_t limit_index);

    /* Called when a limit is triggered during homing/jog; lets kinematics clamp target. */
    void (*on_limit_trigger)(uint8_t limit_index, kin_cart_t *io_target_pos);

    /* Apply machine position set (e.g., after homing) from a Cartesian pose. */
    void (*set_machine_pose)(const kin_cart_t *machine_pos);

    /* Validate a homing cycle request (which axes are being homed together). */
    bool (*validate_homing_axes)(kin_axis_mask_t axes);

    /* Compute homing feedrate adjustments for the selected axes/mode. */
    float (*homing_feedrate)(kin_axis_mask_t axes, float requested_rate, kin_home_mode_t mode);
} kin_iface_t;

/* Global active kinematics interface chosen at init time. */
extern kin_iface_t g_kin;

/* Install/replace the active kinematics implementation. */
void kinematics_install(const kin_iface_t *impl);

/* Convenience helpers to avoid NULL checks all over your core. */
static inline uint8_t kinematics_cart_axes(void)  { return g_kin.cart_axes; }
static inline uint8_t kinematics_joint_axes(void) { return g_kin.joint_axes; }

#ifdef __cplusplus
}
#endif