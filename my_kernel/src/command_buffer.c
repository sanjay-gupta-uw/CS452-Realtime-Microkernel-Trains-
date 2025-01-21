#include "command_buffer.h"

void init_command_ring_buffer(CommandRingBuffer *rb)
{
   rb->head = 0;
   rb->tail = 0;
   rb->size = 0;
}

int add_command_to_buffer(CommandRingBuffer *rb, const CommandType *cmd)
{
   if (rb->size == CMD_RING_BUFFER_MAX_SIZE)
   {
      // Buffer is full
      return 0;
   }

   rb->buffer[rb->head] = *cmd;                          // Copy the command to the buffer
   rb->head = (rb->head + 1) % CMD_RING_BUFFER_MAX_SIZE; // Advance the head (circular)
   rb->size++;                                           // Increase the size

   return 1; // Success
}

int remove_command_from_buffer(CommandRingBuffer *rb, CommandType *cmd)
{
   if (rb->size == 0)
   {
      // Buffer is empty
      return 0;
   }
   if (cmd != NULL)
   {
      *cmd = rb->buffer[rb->tail]; // Copy the value from the buffer
   }
   *cmd = rb->buffer[rb->tail];                          // Copy the command from the buffer
   rb->tail = (rb->tail + 1) % CMD_RING_BUFFER_MAX_SIZE; // Advance the tail (circular)
   rb->size--;                                           // Decrease the size

   return 1; // Success
}

int is_command_buffer_empty(const CommandRingBuffer *rb)
{
   return rb->size == 0;
}

int is_command_buffer_full(const CommandRingBuffer *rb)
{
   return rb->size == CMD_RING_BUFFER_MAX_SIZE;
}
