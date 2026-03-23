#pragma once

#include <cnc_hal.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "gcode.h"
#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t step_pulse_time_us;           /* $0 */
    uint32_t step_idle_delay_ms;           /* $1 */
    uint32_t step_pulse_invert_mask;       /* $2 */
    uint32_t direction_invert_mask;        /* $3 */
    bool step_enable_invert;               /* $4 */
    bool limit_pins_invert;                /* $5 */
    bool probe_pin_invert;                 /* $6 */
    uint32_t status_report_mask;           /* $10 */
    float junction_deviation_mm;           /* $11 */
    float arc_tolerance_mm;                /* $12 */
    bool report_inches;                    /* $13 */
    bool soft_limits_enable;               /* $20 */
    bool hard_limits_enable;               /* $21 */
    bool homing_cycle_enable;              /* $22 */
    uint32_t homing_direction_invert_mask; /* $23 */
    float homing_locate_feed_mm_per_min;   /* $24 */
    float homing_seek_mm_per_min;          /* $25 */
    uint32_t homing_debounce_ms;           /* $26 */
    float homing_pull_off_mm;              /* $27 */
    uint32_t spindle_max_rpm;              /* $30 */
    uint32_t spindle_min_rpm;              /* $31 */
    float max_rate_mm_per_min[3];          /* $110..$112 */
    float accel_mm_per_s2[3];              /* $120..$122 */
    float max_travel_mm[3];                /* $130..$132 */
} grbl_settings_t;

typedef struct {
    gcode_state_t gcode;
    grbl_settings_t settings;
    bool check_mode_enabled;
    bool alarm_lock;
    bool feed_hold;
    uint8_t active_wcs_index; /* 0..5 => G54..G59 */
    char startup_lines[2][PROTOCOL_LINE_MAX + 1];
    float steps_per_mm[HAL_AXIS_MAX];
    uint32_t step_pulse_delay_us;
    bool (*motion_backend)(void *ctx,
                           float start_x,
                           float start_y,
                           float end_x,
                           float end_y,
                           const float *steps_per_mm,
                           uint32_t step_pulse_delay_us);
    void *motion_backend_ctx;
} serial_gcode_bridge_t;

void serial_gcode_bridge_init(serial_gcode_bridge_t *bridge);
void serial_gcode_bridge_set_motion_backend(serial_gcode_bridge_t *bridge,
                                            bool (*backend)(void *ctx,
                                                            float start_x,
                                                            float start_y,
                                                            float end_x,
                                                            float end_y,
                                                            const float *steps_per_mm,
                                                            uint32_t step_pulse_delay_us),
                                            void *backend_ctx);

gcode_status_t serial_gcode_bridge_process_line(serial_gcode_bridge_t *bridge,
                                                const char *line,
                                                char *response,
                                                size_t response_len);

#ifdef __cplusplus
}
#endif
