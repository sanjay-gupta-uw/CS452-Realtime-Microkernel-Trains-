#include "ringbuffer.h"

void init_ring_buffer(RingBuffer *rb)
{
   rb->head = 0;
   rb->tail = 0;
   rb->size = 0;
}

int add_to_buffer(RingBuffer *rb, const int *num)
{
   if (rb->size == RING_BUFFER_MAX_SIZE)
   {
      // Buffer is full
      return 0;
   }

   rb->buffer[rb->head] = *num;                      // Copy the command to the buffer
   rb->head = (rb->head + 1) % RING_BUFFER_MAX_SIZE; // Advance the head (circular)
   rb->size++;                                       // Increase the size

   return 1; // Success
}

int remove_from_buffer(RingBuffer *rb, int *num)
{
   if (rb->size == 0)
   {
      // Buffer is empty
      return 0;
   }

   if (num != NULL)
   {
      *num = rb->buffer[rb->tail]; // Copy the value from the buffer
   }

   rb->tail = (rb->tail + 1) % RING_BUFFER_MAX_SIZE; // Advance the tail (circular)
   rb->size--;                                       // Decrease the size

   return 1; // Success
}

int is_buffer_empty(const RingBuffer *rb)
{
   return rb->size == 0;
}

int is_buffer_full(const RingBuffer *rb)
{
   return rb->size == RING_BUFFER_MAX_SIZE;
}
