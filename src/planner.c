#include "planner.h"
#include "protocol.h"
#include <string.h>
#include <stdlib.h>

// Initialize a planner block with default values
void planner_block_init(planner_block_t *block) {
    if (block == NULL) {
        return;
    }
    
    // Clear all memory to zero
    memset(block, 0, sizeof(planner_block_t));
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

// Initialize a planner queue with given capacity
void planner_queue_init(planner_queue_t *queue, uint32_t capacity) {
    if (queue == NULL) {
        return;
    }
    
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    queue->capacity = capacity;
}

// Add a new block to the end of the queue
// Returns 1 on success, 0 on failure (queue full or NULL parameters)
int planner_enqueue(planner_queue_t *queue, planner_block_t *block) {
    if (queue == NULL || block == NULL) {
        return 0;
    }
    
    // Check if queue is full
    if (queue->capacity > 0 && queue->size >= queue->capacity) {
        return 0; // Queue is full
    }
    
    // Clear the next pointer of the new block
    block->next = NULL;
    
    if (queue->tail == NULL) {
        // Queue is empty, set both head and tail to the new block
        queue->head = block;
        queue->tail = block;
    } else {
        // Add block to the end of the queue
        queue->tail->next = block;
        queue->tail = block;
    }
    
    queue->size++;
    return 1;
}

// Remove and return the block at the front of the queue
// Returns pointer to the block on success, NULL if queue is empty
planner_block_t* planner_dequeue(planner_queue_t *queue) {
    if (queue == NULL || queue->head == NULL) {
        return NULL;
    }
    
    planner_block_t *block = queue->head;
    queue->head = (planner_block_t*)queue->head->next;
    
    if (queue->head == NULL) {
        // Queue is now empty, update tail
        queue->tail = NULL;
    }
    
    queue->size--;
    block->next = NULL; // Clear the next pointer
    return block;
}

// Peek at the block at the front of the queue without removing it
// Returns pointer to the front block, NULL if queue is empty
planner_block_t* planner_peek_front(const planner_queue_t *queue) {
    if (queue == NULL) {
        return NULL;
    }
    
    return queue->head;
}

// Peek at the block at the back of the queue without removing it
// Returns pointer to the back block, NULL if queue is empty
planner_block_t* planner_peek_back(const planner_queue_t *queue) {
    if (queue == NULL) {
        return NULL;
    }
    
    return queue->tail;
}

// Check if the queue is empty
// Returns 1 if empty, 0 if not empty or NULL queue
int planner_is_empty(const planner_queue_t *queue) {
    if (queue == NULL) {
        return 1;
    }
    
    return queue->size == 0;
}

// Clear the queue, removing all blocks
// Note: This does not free the blocks themselves, only removes them from the queue
void planner_queue_clear(planner_queue_t *queue) {
    if (queue == NULL) {
        return;
    }
    
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
}
