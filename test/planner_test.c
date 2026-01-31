#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/planner.h"

// Test initialization of planner block
void test_planner_block_init() {
    printf("Testing planner block initialization...\n");
    
    planner_block_t block;
    
    // Initialize the block
    planner_block_init(&block);
    
    // Verify all fields are initialized correctly
    assert(block.entry_speed == 0.0f);
    assert(block.nominal_speed == 0.0f);
    assert(block.exit_speed == 0.0f);
    assert(block.acceleration == 0.0f);
    assert(block.max_entry_speed == 0.0f);
    assert(block.millimeters == 0.0f);
    assert(block.direction_bits == 0);
    assert(block.step_event_count == 0);
    assert(block.recalculate_flag == 0);
    assert(block.nominal_length_flag == 0);
    assert(block.next == NULL);
    
    printf("[passed]\n");
}

// Test validation of NULL pointer
void test_planner_block_validate_null() {
    printf("Testing planner block validation with NULL...\n");
    
    // NULL pointer should fail validation
    assert(planner_block_validate(NULL) == 0);
    
    printf("[passed]\n");
}

// Test validation of valid planner block
void test_planner_block_validate_valid() {
    printf("Testing planner block validation with valid block...\n");
    
    planner_block_t block;
    planner_block_init(&block);
    
    // Set valid values
    block.entry_speed = 100.0f;
    block.nominal_speed = 200.0f;
    block.exit_speed = 50.0f;
    block.acceleration = 500.0f;
    block.max_entry_speed = 150.0f;
    block.millimeters = 10.0f;
    block.step_event_count = 1000;
    
    // Should pass validation
    assert(planner_block_validate(&block) == 1);
    
    printf("[passed]\n");
}

// Test validation with negative entry speed
void test_planner_block_validate_negative_entry_speed() {
    printf("Testing planner block validation with negative entry speed...\n");
    
    planner_block_t block;
    planner_block_init(&block);
    
    block.entry_speed = -10.0f;  // Invalid
    block.nominal_speed = 200.0f;
    block.exit_speed = 50.0f;
    
    // Should fail validation
    assert(planner_block_validate(&block) == 0);
    
    printf("[passed]\n");
}

// Test validation with negative nominal speed
void test_planner_block_validate_negative_nominal_speed() {
    printf("Testing planner block validation with negative nominal speed...\n");
    
    planner_block_t block;
    planner_block_init(&block);
    
    block.entry_speed = 100.0f;
    block.nominal_speed = -200.0f;  // Invalid
    block.exit_speed = 50.0f;
    
    // Should fail validation
    assert(planner_block_validate(&block) == 0);
    
    printf("[passed]\n");
}

// Test validation with negative exit speed
void test_planner_block_validate_negative_exit_speed() {
    printf("Testing planner block validation with negative exit speed...\n");
    
    planner_block_t block;
    planner_block_init(&block);
    
    block.entry_speed = 100.0f;
    block.nominal_speed = 200.0f;
    block.exit_speed = -50.0f;  // Invalid
    
    // Should fail validation
    assert(planner_block_validate(&block) == 0);
    
    printf("[passed]\n");
}

// Test validation with negative acceleration
void test_planner_block_validate_negative_acceleration() {
    printf("Testing planner block validation with negative acceleration...\n");
    
    planner_block_t block;
    planner_block_init(&block);
    
    block.entry_speed = 100.0f;
    block.nominal_speed = 200.0f;
    block.exit_speed = 50.0f;
    block.acceleration = -500.0f;  // Invalid
    
    // Should fail validation
    assert(planner_block_validate(&block) == 0);
    
    printf("[passed]\n");
}

// Test validation with negative distance
void test_planner_block_validate_negative_distance() {
    printf("Testing planner block validation with negative distance...\n");
    
    planner_block_t block;
    planner_block_init(&block);
    
    block.entry_speed = 100.0f;
    block.nominal_speed = 200.0f;
    block.exit_speed = 50.0f;
    block.millimeters = -10.0f;  // Invalid
    
    // Should fail validation
    assert(planner_block_validate(&block) == 0);
    
    printf("[passed]\n");
}

// Test validation with entry speed exceeding max entry speed
void test_planner_block_validate_entry_exceeds_max() {
    printf("Testing planner block validation with entry speed exceeding max...\n");
    
    planner_block_t block;
    planner_block_init(&block);
    
    block.entry_speed = 200.0f;  // Exceeds max
    block.max_entry_speed = 150.0f;
    block.nominal_speed = 300.0f;
    block.exit_speed = 50.0f;
    
    // Should fail validation
    assert(planner_block_validate(&block) == 0);
    
    printf("[passed]\n");
}

// Test validation with entry speed exceeding nominal speed
void test_planner_block_validate_entry_exceeds_nominal() {
    printf("Testing planner block validation with entry speed exceeding nominal...\n");
    
    planner_block_t block;
    planner_block_init(&block);
    
    block.entry_speed = 250.0f;  // Exceeds nominal
    block.nominal_speed = 200.0f;
    block.exit_speed = 50.0f;
    
    // Should fail validation
    assert(planner_block_validate(&block) == 0);
    
    printf("[passed]\n");
}

// Test validation with exit speed exceeding nominal speed
void test_planner_block_validate_exit_exceeds_nominal() {
    printf("Testing planner block validation with exit speed exceeding nominal...\n");
    
    planner_block_t block;
    planner_block_init(&block);
    
    block.entry_speed = 100.0f;
    block.nominal_speed = 200.0f;
    block.exit_speed = 250.0f;  // Exceeds nominal
    
    // Should fail validation
    assert(planner_block_validate(&block) == 0);
    
    printf("[passed]\n");
}

// Test that all required fields exist in the structure
void test_planner_block_has_required_fields() {
    printf("Testing that planner block has all required fields...\n");
    
    planner_block_t block;
    
    // Test that we can access all required fields
    block.entry_speed = 100.0f;
    block.nominal_speed = 200.0f;
    block.exit_speed = 50.0f;
    block.acceleration = 500.0f;
    block.max_entry_speed = 150.0f;
    block.millimeters = 10.0f;
    block.direction_bits = 0xFF;
    block.step_event_count = 1000;
    block.recalculate_flag = 1;
    block.nominal_length_flag = 1;
    block.next = NULL;
    
    // Verify values were set correctly
    assert(block.entry_speed == 100.0f);
    assert(block.nominal_speed == 200.0f);
    assert(block.exit_speed == 50.0f);
    assert(block.acceleration == 500.0f);
    assert(block.max_entry_speed == 150.0f);
    assert(block.millimeters == 10.0f);
    assert(block.direction_bits == 0xFF);
    assert(block.step_event_count == 1000);
    assert(block.recalculate_flag == 1);
    assert(block.nominal_length_flag == 1);
    assert(block.next == NULL);
    
    printf("[passed]\n");
}

// Test planner block with zero nominal speed (edge case)
void test_planner_block_zero_nominal_speed() {
    printf("Testing planner block with zero nominal speed...\n");
    
    planner_block_t block;
    planner_block_init(&block);
    
    // Zero nominal speed should be valid
    // (validation only checks if entry/exit exceed nominal when nominal > 0)
    block.entry_speed = 0.0f;
    block.nominal_speed = 0.0f;
    block.exit_speed = 0.0f;
    block.acceleration = 100.0f;
    
    assert(planner_block_validate(&block) == 1);
    
    printf("[passed]\n");
}

// Test planner block with complete stop (all speeds zero)
void test_planner_block_complete_stop() {
    printf("Testing planner block with complete stop (all speeds zero)...\n");
    
    planner_block_t block;
    planner_block_init(&block);
    
    // All speeds zero should be valid (represents a complete stop)
    block.entry_speed = 0.0f;
    block.nominal_speed = 0.0f;
    block.exit_speed = 0.0f;
    
    assert(planner_block_validate(&block) == 1);
    
    printf("[passed]\n");
}

// ===== Queue Tests =====

// Test queue initialization
void test_planner_queue_init() {
    printf("Testing planner queue initialization...\n");
    
    planner_queue_t queue;
    planner_queue_init(&queue, 10);
    
    assert(queue.head == NULL);
    assert(queue.tail == NULL);
    assert(queue.size == 0);
    assert(queue.capacity == 10);
    
    printf("[passed]\n");
}

// Test enqueuing a single block
void test_planner_queue_enqueue_single() {
    printf("Testing planner queue enqueue single block...\n");
    
    planner_queue_t queue;
    planner_queue_init(&queue, 10);
    
    planner_block_t block1;
    planner_block_init(&block1);
    block1.nominal_speed = 100.0f;
    
    assert(planner_enqueue(&queue, &block1) == 1);
    assert(queue.size == 1);
    assert(queue.head == &block1);
    assert(queue.tail == &block1);
    
    printf("[passed]\n");
}

// Test enqueuing multiple blocks
void test_planner_queue_enqueue_multiple() {
    printf("Testing planner queue enqueue multiple blocks...\n");
    
    planner_queue_t queue;
    planner_queue_init(&queue, 10);
    
    planner_block_t block1, block2, block3;
    planner_block_init(&block1);
    planner_block_init(&block2);
    planner_block_init(&block3);
    
    block1.nominal_speed = 100.0f;
    block2.nominal_speed = 200.0f;
    block3.nominal_speed = 300.0f;
    
    assert(planner_enqueue(&queue, &block1) == 1);
    assert(planner_enqueue(&queue, &block2) == 1);
    assert(planner_enqueue(&queue, &block3) == 1);
    
    assert(queue.size == 3);
    assert(queue.head == &block1);
    assert(queue.tail == &block3);
    
    printf("[passed]\n");
}

// Test dequeuing a single block
void test_planner_queue_dequeue_single() {
    printf("Testing planner queue dequeue single block...\n");
    
    planner_queue_t queue;
    planner_queue_init(&queue, 10);
    
    planner_block_t block1;
    planner_block_init(&block1);
    block1.nominal_speed = 100.0f;
    
    planner_enqueue(&queue, &block1);
    
    planner_block_t *dequeued = planner_dequeue(&queue);
    assert(dequeued == &block1);
    assert(dequeued->nominal_speed == 100.0f);
    assert(queue.size == 0);
    assert(queue.head == NULL);
    assert(queue.tail == NULL);
    
    printf("[passed]\n");
}

// Test dequeuing multiple blocks (FIFO order)
void test_planner_queue_dequeue_multiple() {
    printf("Testing planner queue dequeue multiple blocks (FIFO)...\n");
    
    planner_queue_t queue;
    planner_queue_init(&queue, 10);
    
    planner_block_t block1, block2, block3;
    planner_block_init(&block1);
    planner_block_init(&block2);
    planner_block_init(&block3);
    
    block1.nominal_speed = 100.0f;
    block2.nominal_speed = 200.0f;
    block3.nominal_speed = 300.0f;
    
    planner_enqueue(&queue, &block1);
    planner_enqueue(&queue, &block2);
    planner_enqueue(&queue, &block3);
    
    planner_block_t *dequeued1 = planner_dequeue(&queue);
    assert(dequeued1 == &block1);
    assert(dequeued1->nominal_speed == 100.0f);
    assert(queue.size == 2);
    
    planner_block_t *dequeued2 = planner_dequeue(&queue);
    assert(dequeued2 == &block2);
    assert(dequeued2->nominal_speed == 200.0f);
    assert(queue.size == 1);
    
    planner_block_t *dequeued3 = planner_dequeue(&queue);
    assert(dequeued3 == &block3);
    assert(dequeued3->nominal_speed == 300.0f);
    assert(queue.size == 0);
    assert(queue.head == NULL);
    assert(queue.tail == NULL);
    
    printf("[passed]\n");
}

// Test dequeuing from empty queue
void test_planner_queue_dequeue_empty() {
    printf("Testing planner queue dequeue from empty queue...\n");
    
    planner_queue_t queue;
    planner_queue_init(&queue, 10);
    
    planner_block_t *dequeued = planner_dequeue(&queue);
    assert(dequeued == NULL);
    assert(queue.size == 0);
    
    printf("[passed]\n");
}

// Test enqueuing when queue is full
void test_planner_queue_enqueue_full() {
    printf("Testing planner queue enqueue when full...\n");
    
    planner_queue_t queue;
    planner_queue_init(&queue, 2); // Small capacity
    
    planner_block_t block1, block2, block3;
    planner_block_init(&block1);
    planner_block_init(&block2);
    planner_block_init(&block3);
    
    assert(planner_enqueue(&queue, &block1) == 1);
    assert(planner_enqueue(&queue, &block2) == 1);
    assert(planner_enqueue(&queue, &block3) == 0); // Should fail
    
    assert(queue.size == 2);
    
    printf("[passed]\n");
}

// Test peeking at front of queue
void test_planner_queue_peek_front() {
    printf("Testing planner queue peek front...\n");
    
    planner_queue_t queue;
    planner_queue_init(&queue, 10);
    
    planner_block_t block1, block2;
    planner_block_init(&block1);
    planner_block_init(&block2);
    
    block1.nominal_speed = 100.0f;
    block2.nominal_speed = 200.0f;
    
    planner_enqueue(&queue, &block1);
    planner_enqueue(&queue, &block2);
    
    planner_block_t *front = planner_peek_front(&queue);
    assert(front == &block1);
    assert(front->nominal_speed == 100.0f);
    assert(queue.size == 2); // Size should not change
    
    printf("[passed]\n");
}

// Test peeking at back of queue
void test_planner_queue_peek_back() {
    printf("Testing planner queue peek back...\n");
    
    planner_queue_t queue;
    planner_queue_init(&queue, 10);
    
    planner_block_t block1, block2;
    planner_block_init(&block1);
    planner_block_init(&block2);
    
    block1.nominal_speed = 100.0f;
    block2.nominal_speed = 200.0f;
    
    planner_enqueue(&queue, &block1);
    planner_enqueue(&queue, &block2);
    
    planner_block_t *back = planner_peek_back(&queue);
    assert(back == &block2);
    assert(back->nominal_speed == 200.0f);
    assert(queue.size == 2); // Size should not change
    
    printf("[passed]\n");
}

// Test peeking at empty queue
void test_planner_queue_peek_empty() {
    printf("Testing planner queue peek on empty queue...\n");
    
    planner_queue_t queue;
    planner_queue_init(&queue, 10);
    
    assert(planner_peek_front(&queue) == NULL);
    assert(planner_peek_back(&queue) == NULL);
    
    printf("[passed]\n");
}

// Test is_empty function
void test_planner_is_empty() {
    printf("Testing planner is_empty...\n");
    
    planner_queue_t queue;
    planner_queue_init(&queue, 10);
    
    assert(planner_is_empty(&queue) == 1);
    
    planner_block_t block1;
    planner_block_init(&block1);
    planner_enqueue(&queue, &block1);
    
    assert(planner_is_empty(&queue) == 0);
    
    planner_dequeue(&queue);
    assert(planner_is_empty(&queue) == 1);
    
    printf("[passed]\n");
}

// Test clearing the queue
void test_planner_queue_clear() {
    printf("Testing planner queue clear...\n");
    
    planner_queue_t queue;
    planner_queue_init(&queue, 10);
    
    planner_block_t block1, block2, block3;
    planner_block_init(&block1);
    planner_block_init(&block2);
    planner_block_init(&block3);
    
    planner_enqueue(&queue, &block1);
    planner_enqueue(&queue, &block2);
    planner_enqueue(&queue, &block3);
    
    assert(queue.size == 3);
    
    planner_queue_clear(&queue);
    
    assert(queue.size == 0);
    assert(queue.head == NULL);
    assert(queue.tail == NULL);
    assert(planner_is_empty(&queue) == 1);
    
    printf("[passed]\n");
}

// Test clearing an empty queue
void test_planner_queue_clear_empty() {
    printf("Testing planner queue clear on empty queue...\n");
    
    planner_queue_t queue;
    planner_queue_init(&queue, 10);
    
    planner_queue_clear(&queue);
    
    assert(queue.size == 0);
    assert(queue.head == NULL);
    assert(queue.tail == NULL);
    
    printf("[passed]\n");
}

// Test enqueuing after clearing
void test_planner_queue_enqueue_after_clear() {
    printf("Testing planner queue enqueue after clear...\n");
    
    planner_queue_t queue;
    planner_queue_init(&queue, 10);
    
    planner_block_t block1, block2;
    planner_block_init(&block1);
    planner_block_init(&block2);
    
    planner_enqueue(&queue, &block1);
    planner_queue_clear(&queue);
    
    assert(planner_enqueue(&queue, &block2) == 1);
    assert(queue.size == 1);
    assert(queue.head == &block2);
    assert(queue.tail == &block2);
    
    printf("[passed]\n");
}

// Test NULL parameter handling
void test_planner_queue_null_parameters() {
    printf("Testing planner queue NULL parameter handling...\n");
    
    planner_queue_t queue;
    planner_block_t block;
    
    // NULL queue
    planner_queue_init(NULL, 10);
    assert(planner_enqueue(NULL, &block) == 0);
    assert(planner_dequeue(NULL) == NULL);
    assert(planner_peek_front(NULL) == NULL);
    assert(planner_peek_back(NULL) == NULL);
    assert(planner_is_empty(NULL) == 1);
    planner_queue_clear(NULL);
    
    // NULL block with valid queue
    planner_queue_init(&queue, 10);
    assert(planner_enqueue(&queue, NULL) == 0);
    
    printf("[passed]\n");
}

// Main function to execute all test cases
int main() {
    printf("=== Running Planner Block Tests ===\n\n");
    
    // Run all tests
    test_planner_block_init();
    test_planner_block_validate_null();
    test_planner_block_validate_valid();
    test_planner_block_validate_negative_entry_speed();
    test_planner_block_validate_negative_nominal_speed();
    test_planner_block_validate_negative_exit_speed();
    test_planner_block_validate_negative_acceleration();
    test_planner_block_validate_negative_distance();
    test_planner_block_validate_entry_exceeds_max();
    test_planner_block_validate_entry_exceeds_nominal();
    test_planner_block_validate_exit_exceeds_nominal();
    test_planner_block_has_required_fields();
    test_planner_block_zero_nominal_speed();
    test_planner_block_complete_stop();
    
    printf("\n=== All planner block tests passed! ===\n");
    
    // Run queue tests
    printf("\n=== Running Planner Queue Tests ===\n\n");
    
    test_planner_queue_init();
    test_planner_queue_enqueue_single();
    test_planner_queue_enqueue_multiple();
    test_planner_queue_dequeue_single();
    test_planner_queue_dequeue_multiple();
    test_planner_queue_dequeue_empty();
    test_planner_queue_enqueue_full();
    test_planner_queue_peek_front();
    test_planner_queue_peek_back();
    test_planner_queue_peek_empty();
    test_planner_is_empty();
    test_planner_queue_clear();
    test_planner_queue_clear_empty();
    test_planner_queue_enqueue_after_clear();
    test_planner_queue_null_parameters();
    
    printf("\n=== All planner queue tests passed! ===\n");
    
    return 0;
}
