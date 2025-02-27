#ifndef _ring_buffer_h_
#define _ring_buffer_h_

#define RING_BUFFER_MAX_SIZE 256
#include <stdlib.h>

typedef struct
{
   int buffer[RING_BUFFER_MAX_SIZE]; // Fixed-size buffer
   int head;                         // Index of the next write
   int tail;                         // Index of the next read
   int size;                         // Current size of the buffer
} RingBuffer;

// Function declarations
void init_ring_buffer(RingBuffer *rb);
int add_to_buffer(RingBuffer *rb, const int *num);
int get_from_buffer(const RingBuffer *rb, int index, int *num);
int remove_from_buffer(RingBuffer *rb, int *num);
int is_buffer_empty(const RingBuffer *rb);
int is_buffer_full(const RingBuffer *rb);

#endif // _command_buffer_h_
