#ifndef _command_buffer_h_
#define _command_buffer_h_

#include "command.h"

#define CMD_RING_BUFFER_MAX_SIZE 256

typedef struct
{
   CommandType buffer[CMD_RING_BUFFER_MAX_SIZE]; // Fixed-size buffer
   int head;                                     // Index of the next write
   int tail;                                     // Index of the next read
   int size;                                     // Current size of the buffer
} CommandRingBuffer;

// Function declarations
void init_command_ring_buffer(CommandRingBuffer *rb);
int add_command_to_buffer(CommandRingBuffer *rb, const CommandType *cmd);
int remove_command_from_buffer(CommandRingBuffer *rb, CommandType *cmd);
int is_command_buffer_empty(const CommandRingBuffer *rb);
int is_command_buffer_full(const CommandRingBuffer *rb);

#endif // _command_buffer_h_
