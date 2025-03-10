// ringbuffer.h
#ifndef _ring_buffer_h_
#define _ring_buffer_h_

#define RING_BUFFER_MAX_SIZE 256
#include <stdint.h>
#include <stddef.h>

typedef struct {
    int sensor_idx;
    uint32_t minutes;
    uint32_t seconds;
    uint32_t tenths;
} SensorEvent;

typedef struct {
    SensorEvent buffer[RING_BUFFER_MAX_SIZE];
    int head;
    int tail;
    int size;
} RingBuffer;

void init_ring_buffer(RingBuffer *rb);
int add_to_buffer(RingBuffer *rb, const SensorEvent *event);
int get_from_buffer(const RingBuffer *rb, int index, SensorEvent *event);
int remove_from_buffer(RingBuffer *rb, SensorEvent *event);
int is_buffer_empty(const RingBuffer *rb);
int is_buffer_full(const RingBuffer *rb);

#endif