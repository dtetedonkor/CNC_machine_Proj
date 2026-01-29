/* kinematics.c - minimal installer + safe defaults */

#include "kinematics.h"
#include <string.h>

static void steps_to_cart_stub(const kin_steps_t *steps, kin_cart_t *out_cart) {
    (void)steps;
    if (out_cart) {
        for (uint8_t i = 0; i < KIN_MAX_CART_AXES; i++) out_cart->v[i] = 0.0f;
    }
}

static bool cart_to_joint_stub(const kin_cart_t *cart, kin_joint_t *out_joint) {
    (void)cart;
    if (out_joint) {
        for (uint8_t i = 0; i < KIN_MAX_JOINT_AXES; i++) out_joint->v[i] = 0.0f;
    }
    return false;
}

static bool joint_to_cart_stub(const kin_joint_t *joint, kin_cart_t *out_cart) {
    (void)joint;
    if (out_cart) {
        for (uint8_t i = 0; i < KIN_MAX_CART_AXES; i++) out_cart->v[i] = 0.0f;
    }
    return false;
}

static bool segment_move_stub(const kin_cart_t *t, const kin_cart_t *c,
                              const kin_motion_hint_t *h, bool init,
                              kin_cart_t *out_next)
{
    (void)h;
    if (!t || !c || !out_next) return false;

    /* Default behavior: no segmentation; return target once on init. */
    if (init) {
        *out_next = *t;
        return true;
    }
    return false;
}

static kin_axis_mask_t limit_index_to_axes_stub(uint8_t idx) {
    (void)idx;
    return 0u;
}

static void on_limit_trigger_stub(uint8_t idx, kin_cart_t *io_target_pos) {
    (void)idx;
    (void)io_target_pos;
}

static void set_machine_pose_stub(const kin_cart_t *machine_pos) {
    (void)machine_pos;
}

static bool validate_homing_axes_stub(kin_axis_mask_t axes) {
    (void)axes;
    return false;
}

static float homing_feedrate_stub(kin_axis_mask_t axes, float req, kin_home_mode_t mode) {
    (void)axes; (void)mode;
    return req;
}

/* Active interface (defaults to safe stubs) */
kin_iface_t g_kin = {
    .cart_axes = (uint8_t)KIN_MAX_CART_AXES,
    .joint_axes = (uint8_t)KIN_MAX_JOINT_AXES,

    .steps_to_cart = steps_to_cart_stub,
    .cart_to_joint = cart_to_joint_stub,
    .joint_to_cart = joint_to_cart_stub,
    .segment_move = segment_move_stub,

    .limit_index_to_axes = limit_index_to_axes_stub,
    .on_limit_trigger = on_limit_trigger_stub,
    .set_machine_pose = set_machine_pose_stub,
    .validate_homing_axes = validate_homing_axes_stub,
    .homing_feedrate = homing_feedrate_stub
};

void kinematics_install(const kin_iface_t *impl) {
    if (!impl) return;

    /* Copy entire vtable; user must fill all function pointers they care about. */
    g_kin = *impl;

    /* Defensive defaults if user left any hooks NULL. */
    if (!g_kin.steps_to_cart)        g_kin.steps_to_cart = steps_to_cart_stub;
    if (!g_kin.cart_to_joint)        g_kin.cart_to_joint = cart_to_joint_stub;
    if (!g_kin.joint_to_cart)        g_kin.joint_to_cart = joint_to_cart_stub;
    if (!g_kin.segment_move)         g_kin.segment_move = segment_move_stub;
    if (!g_kin.limit_index_to_axes)  g_kin.limit_index_to_axes = limit_index_to_axes_stub;
    if (!g_kin.on_limit_trigger)     g_kin.on_limit_trigger = on_limit_trigger_stub;
    if (!g_kin.set_machine_pose)     g_kin.set_machine_pose = set_machine_pose_stub;
    if (!g_kin.validate_homing_axes) g_kin.validate_homing_axes = validate_homing_axes_stub;
    if (!g_kin.homing_feedrate)      g_kin.homing_feedrate = homing_feedrate_stub;

    /* Clamp axis counts to configured maxima */
    if (g_kin.cart_axes == 0 || g_kin.cart_axes > KIN_MAX_CART_AXES)
        g_kin.cart_axes = (uint8_t)KIN_MAX_CART_AXES;

    if (g_kin.joint_axes == 0 || g_kin.joint_axes > KIN_MAX_JOINT_AXES)
        g_kin.joint_axes = (uint8_t)KIN_MAX_JOINT_AXES;
}