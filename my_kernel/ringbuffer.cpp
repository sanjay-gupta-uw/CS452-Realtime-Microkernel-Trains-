#include "ringbuffer.h"

#include "command.h"
// instantiate the template class
template class RingBuffer<Command>;

template <typename T>
RingBuffer<T>::RingBuffer()
{
   head = 0;
   tail = 0;
   size = 0;
}

template <typename T>
RingBuffer<T>::~RingBuffer()
{
}

template <typename T>
int RingBuffer<T>::Push(const T *item)
{
   if (size == RING_BUFFER_MAX_SIZE)
   {
      // Buffer is full
      return -1;
   }

   buffer[head] = *item;                     // Copy the command to the buffer
   head = (head + 1) % RING_BUFFER_MAX_SIZE; // Advance the head (circular)
   size++;                                   // Increase the size

   return 0; // Success
}

template <typename T>
int RingBuffer<T>::Pop(T *item)
{
   if (size == 0)
   {
      // Buffer is empty
      return -1;
   }

   *item = buffer[tail]; // Copy the command from the buffer

   tail = (tail + 1) % RING_BUFFER_MAX_SIZE; // Advance the tail (circular)
   size--;                                   // Decrease the size

   return 0; // Success
}

template <typename T>
bool RingBuffer<T>::IsEmpty()
{
   return size == 0;
}

template <typename T>
bool RingBuffer<T>::IsFull()
{
   return size == RING_BUFFER_MAX_SIZE;
}
