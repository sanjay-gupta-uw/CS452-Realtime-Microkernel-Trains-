// ringbuffer.c
#include "ringbuffer.h"

void init_ring_buffer(RingBuffer *rb) {
    rb->head = 0;
    rb->tail = 0;
    rb->size = 0;
}

int add_to_buffer(RingBuffer *rb, const SensorEvent *event) {
    if (rb->size == RING_BUFFER_MAX_SIZE) return 0;
    rb->buffer[rb->head] = *event;
    rb->head = (rb->head + 1) % RING_BUFFER_MAX_SIZE;
    rb->size++;
    return 1;
}

int get_from_buffer(const RingBuffer *rb, int index, SensorEvent *event) {
    if (index < 0 || index >= rb->size) return 0;
    int physical_idx = (rb->tail + index) % RING_BUFFER_MAX_SIZE;
    *event = rb->buffer[physical_idx];
    return 1;
}

int remove_from_buffer(RingBuffer *rb, SensorEvent *event) {
    if (rb->size == 0) return 0;
    if (event != NULL) *event = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % RING_BUFFER_MAX_SIZE;
    rb->size--;
    return 1;
}

int is_buffer_empty(const RingBuffer *rb) { return rb->size == 0; }
int is_buffer_full(const RingBuffer *rb) { return rb->size == RING_BUFFER_MAX_SIZE; }