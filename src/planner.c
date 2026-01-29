#include "planner.h"
#include <string.h>

// Initialize a planner block with default values
void planner_block_init(planner_block_t *block) {
    if (block == NULL) {
        return;
    }
    
    // Clear all memory
    memset(block, 0, sizeof(planner_block_t));
    
    // Set default values
    block->entry_speed = 0.0f;
    block->nominal_speed = 0.0f;
    block->exit_speed = 0.0f;
    block->acceleration = 0.0f;
    block->max_entry_speed = 0.0f;
    block->millimeters = 0.0f;
    block->direction_bits = 0;
    block->step_event_count = 0;
    block->recalculate_flag = 0;
    block->nominal_length_flag = 0;
    block->next = NULL;
}

// Validate that a planner block has all required information
// Returns 1 if valid, 0 if invalid
int planner_block_validate(const planner_block_t *block) {
    if (block == NULL) {
        return 0; // Invalid: NULL pointer
    }
    
    // Check that speeds are non-negative
    if (block->entry_speed < 0.0f) {
        return 0; // Invalid: negative entry speed
    }
    
    if (block->nominal_speed < 0.0f) {
        return 0; // Invalid: negative nominal speed
    }
    
    if (block->exit_speed < 0.0f) {
        return 0; // Invalid: negative exit speed
    }
    
    // Check that acceleration is non-negative
    if (block->acceleration < 0.0f) {
        return 0; // Invalid: negative acceleration
    }
    
    // Check that max_entry_speed is non-negative
    if (block->max_entry_speed < 0.0f) {
        return 0; // Invalid: negative max entry speed
    }
    
    // Check that millimeters is non-negative
    if (block->millimeters < 0.0f) {
        return 0; // Invalid: negative distance
    }
    
    // Check that entry_speed does not exceed max_entry_speed (if max is set)
    if (block->max_entry_speed > 0.0f && block->entry_speed > block->max_entry_speed) {
        return 0; // Invalid: entry speed exceeds maximum
    }
    
    // Check that entry_speed and exit_speed do not exceed nominal_speed
    if (block->nominal_speed > 0.0f) {
        if (block->entry_speed > block->nominal_speed) {
            return 0; // Invalid: entry speed exceeds nominal speed
        }
        if (block->exit_speed > block->nominal_speed) {
            return 0; // Invalid: exit speed exceeds nominal speed
        }
    }
    
    return 1; // Valid block
}
