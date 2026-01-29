/* kin_corexy.c */

#include "kin_corexy.h"
#include "planner.h"
#include "hal.h"
#include <math.h>
#include <string.h>

/* Internal config + internal machine pose.
 * In a real firmware you might store machine pose elsewhere; here we keep it simple.
 */
static kin_corexy_cfg_t s_cfg;
static kin_cart_t s_machine_pose_cart; /* current machine position in Cartesian */

/* ----------------- small helpers ----------------- */

static float maybe_invert(bool inv, float x) { return inv ? -x : x; }

static void apply_cart_invert(const kin_corexy_cfg_t *c, kin_cart_t *p) {
    for (uint8_t i = 0; i < KIN_MAX_CART_AXES; i++) p->v[i] = maybe_invert(c->invert_cart[i], p->v[i]);
}

static void apply_joint_invert(const kin_corexy_cfg_t *c, kin_joint_t *p) {
    for (uint8_t i = 0; i < KIN_MAX_JOINT_AXES; i++) p->v[i] = maybe_invert(c->invert_joint[i], p->v[i]);
}

static void apply_joint_invert_steps(const kin_corexy_cfg_t *c, kin_steps_t *p) {
    for (uint8_t i = 0; i < KIN_MAX_JOINT_AXES; i++) {
        if (c->invert_joint[i]) p->v[i] = -p->v[i];
    }
}

/* ----------------- interface functions ----------------- */

static void corexy_steps_to_cart(const kin_steps_t *steps, kin_cart_t *out_cart)
{
    /* steps -> joint mm -> cart mm */
    kin_steps_t st = *steps;
    apply_joint_invert_steps(&s_cfg, &st);

    /* Convert steps to "joint mm-equivalent" */
    float a = 0.0f, b = 0.0f, z = 0.0f;

    if (s_cfg.steps_per_mm[0] != 0.0f) a = (float)st.v[0] / s_cfg.steps_per_mm[0];
    if (s_cfg.steps_per_mm[1] != 0.0f) b = (float)st.v[1] / s_cfg.steps_per_mm[1];
    if (s_cfg.steps_per_mm[2] != 0.0f) z = (float)st.v[2] / s_cfg.steps_per_mm[2];

    /* Inverse CoreXY */
    out_cart->v[0] = 0.5f * (a + b); /* X */
    out_cart->v[1] = 0.5f * (a - b); /* Y */
    out_cart->v[2] = z;              /* Z passes through */

    apply_cart_invert(&s_cfg, out_cart);
}

static bool corexy_cart_to_joint(const kin_cart_t *cart_in, kin_joint_t *out_joint)
{
    kin_cart_t cart = *cart_in;
    apply_cart_invert(&s_cfg, &cart); /* bring into "mechanism space" */

    const float x = cart.v[0];
    const float y = cart.v[1];
    const float z = cart.v[2];

    /* Forward CoreXY */
    out_joint->v[0] = x + y; /* A */
    out_joint->v[1] = x - y; /* B */
    out_joint->v[2] = z;     /* Z */
    out_joint->v[3] = 0.0f;  /* AUX unused by default */

    apply_joint_invert(&s_cfg, out_joint);
    return true;
}

static bool corexy_joint_to_cart(const kin_joint_t *joint_in, kin_cart_t *out_cart)
{
    kin_joint_t j = *joint_in;
    apply_joint_invert(&s_cfg, &j); /* back to mechanism direction */

    const float a = j.v[0];
    const float b = j.v[1];
    const float z = j.v[2];

    out_cart->v[0] = 0.5f * (a + b);
    out_cart->v[1] = 0.5f * (a - b);
    out_cart->v[2] = z;

    apply_cart_invert(&s_cfg, out_cart);
    return true;
}

static bool corexy_segment_move(const kin_cart_t *cart_target,
                                const kin_cart_t *cart_current,
                                const kin_motion_hint_t *hint,
                                bool init,
                                kin_cart_t *out_cart_next)
{
    (void)hint;

    /* CoreXY is linear in joint space, so segmentation is not required.
       We support optional segmentation by max segment length in Cartesian space. */

    static kin_cart_t s_tgt;
    static kin_cart_t s_cur;
    static uint16_t   s_n;
    static uint16_t   s_i;

    if (!cart_target || !cart_current || !out_cart_next) return false;

    if (s_cfg.max_segment_len_mm <= 0.0f) {
        if (init) { *out_cart_next = *cart_target; return true; }
        return false;
    }

    if (init) {
        s_tgt = *cart_target;
        s_cur = *cart_current;
        s_i = 0;

        const float dx = s_tgt.v[0] - s_cur.v[0];
        const float dy = s_tgt.v[1] - s_cur.v[1];
        const float dz = s_tgt.v[2] - s_cur.v[2];

        /* cheap L-infinity segment length (max axis delta) to avoid sqrt */
        float maxd = dx; if (maxd < 0) maxd = -maxd;
        float ay = dy; if (ay < 0) ay = -ay; if (ay > maxd) maxd = ay;
        float az = dz; if (az < 0) az = -az; if (az > maxd) maxd = az;

        s_n = (uint16_t)(maxd / s_cfg.max_segment_len_mm);
        if (s_n == 0) s_n = 1;
        if (s_n > 10000u) s_n = 10000u; /* sanity clamp */
    }

    if (s_i >= s_n) return false;

    /* produce next point (including final target at i=s_n-1) */
    s_i++;
    const float t = (float)s_i / (float)s_n;

    out_cart_next->v[0] = s_cur.v[0] + (s_tgt.v[0] - s_cur.v[0]) * t;
    out_cart_next->v[1] = s_cur.v[1] + (s_tgt.v[1] - s_cur.v[1]) * t;
    out_cart_next->v[2] = s_cur.v[2] + (s_tgt.v[2] - s_cur.v[2]) * t;

    return true;
}

/* Simple mapping for limit indices to axes:
 * idx: 0=Xmin, 1=Xmax, 2=Ymin, 3=Ymax, 4=Zmin, 5=Zmax
 * Return axis bits: bit0=X, bit1=Y, bit2=Z
 */
static kin_axis_mask_t corexy_limit_index_to_axes(uint8_t idx)
{
    switch (idx) {
        case 0: case 1: return 1u << 0; /* X */
        case 2: case 3: return 1u << 1; /* Y */
        case 4: case 5: return 1u << 2; /* Z */
        default:        return 0u;
    }
}

static void corexy_on_limit_trigger(uint8_t idx, kin_cart_t *io_target_pos)
{
    (void)idx;
    /* Minimal: do nothing. A more advanced impl would clamp target along that axis. */
    (void)io_target_pos;
}

static void corexy_set_machine_pose(const kin_cart_t *machine_pos)
{
    if (!machine_pos) return;
    s_machine_pose_cart = *machine_pos;
}

static bool corexy_validate_homing_axes(kin_axis_mask_t axes)
{
    /* Allow homing any subset of XYZ. */
    const kin_axis_mask_t valid = (1u << 0) | (1u << 1) | (1u << 2);
    return (axes != 0u) && ((axes & ~valid) == 0u);
}

static float corexy_homing_feedrate(kin_axis_mask_t axes, float req, kin_home_mode_t mode)
{
    (void)axes;

    if (mode == KIN_HOME_FAST && s_cfg.home_fast_mm_min > 0.0f) return s_cfg.home_fast_mm_min;
    if (mode == KIN_HOME_SLOW && s_cfg.home_slow_mm_min > 0.0f) return s_cfg.home_slow_mm_min;
    return req;
}

/* ----------------- public install/config ----------------- */

void kin_corexy_set_cfg(const kin_corexy_cfg_t *cfg)
{
    if (!cfg) return;
    s_cfg = *cfg;
}

void kin_corexy_get_cfg(kin_corexy_cfg_t *out)
{
    if (!out) return;
    *out = s_cfg;
}

void kin_corexy_install(const kin_corexy_cfg_t *cfg)
{
    /* Defaults */
    memset(&s_cfg, 0, sizeof(s_cfg));
    s_cfg.steps_per_mm[0] = 80.0f; /* A */
    s_cfg.steps_per_mm[1] = 80.0f; /* B */
    s_cfg.steps_per_mm[2] = 400.0f;/* Z */
    s_cfg.steps_per_mm[3] = 0.0f;  /* AUX unused */
    s_cfg.max_segment_len_mm = 0.0f; /* disabled by default */
    s_cfg.home_fast_mm_min = 800.0f;
    s_cfg.home_slow_mm_min = 200.0f;

    if (cfg) s_cfg = *cfg;

    /* Build interface */
    kin_iface_t impl = {
        .cart_axes = 3,
        .joint_axes = 3, /* A,B,Z (AUX unused) */

        .steps_to_cart = corexy_steps_to_cart,
        .cart_to_joint = corexy_cart_to_joint,
        .joint_to_cart = corexy_joint_to_cart,
        .segment_move = corexy_segment_move,

        .limit_index_to_axes = corexy_limit_index_to_axes,
        .on_limit_trigger = corexy_on_limit_trigger,
        .set_machine_pose = corexy_set_machine_pose,
        .validate_homing_axes = corexy_validate_homing_axes,
        .homing_feedrate = corexy_homing_feedrate
    };

    kinematics_install(&impl);
}