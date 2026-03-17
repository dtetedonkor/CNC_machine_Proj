#ifndef PLANNER_H
#define PLANNER_H

#include <stdint.h>

// Planner block structure
// This structure contains all the information needed for motion planning
typedef struct {
    // Speed parameters
    float entry_speed;        // Entry speed for this block (mm/min)
    float nominal_speed;      // Maximum speed this block can achieve (mm/min)
    float exit_speed;         // Exit speed for this block (mm/min)
    
    // Acceleration parameters
    float acceleration;       // Maximum acceleration for this block (mm/min^2)
    float max_entry_speed;    // Maximum allowable entry speed (mm/min)
    
    // Distance and time
    float millimeters;        // Total distance to travel in this block (mm)
    
    // Direction and step counts
    uint8_t direction_bits;   // Direction bits for each axis
    uint32_t step_event_count; // Number of step events for this block
    
    // Status flags
    uint8_t recalculate_flag; // Flag to indicate block needs recalculation
    uint8_t nominal_length_flag; // Flag to indicate block is running at nominal speed
    
    // Pointer to next block (for linked list)
    void *next;               // Pointer to next planner block
    
} planner_block_t;

// Queue structure for managing planner blocks
typedef struct {
    planner_block_t *head;    // Pointer to the front of the queue
    planner_block_t *tail;    // Pointer to the back of the queue
    uint32_t size;            // Current number of blocks in the queue
    uint32_t capacity;        // Maximum number of blocks allowed in the queue
} planner_queue_t;

// Function declarations - Block operations
void planner_block_init(planner_block_t *block);
int planner_block_validate(const planner_block_t *block);

// Function declarations - Queue operations
void planner_queue_init(planner_queue_t *queue, uint32_t capacity);
int planner_enqueue(planner_queue_t *queue, planner_block_t *block);
planner_block_t* planner_dequeue(planner_queue_t *queue);
planner_block_t* planner_peek_front(const planner_queue_t *queue);
planner_block_t* planner_peek_back(const planner_queue_t *queue);
int planner_is_empty(const planner_queue_t *queue);
void planner_queue_clear(planner_queue_t *queue);

#endif // PLANNER_H
