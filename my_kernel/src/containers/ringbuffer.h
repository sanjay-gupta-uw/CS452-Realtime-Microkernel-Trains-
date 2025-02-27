#ifndef _ring_buffer_h_
#define _ring_buffer_h_

#define RING_BUFFER_MAX_SIZE 256

template <typename T>
class RingBuffer
{
private:
   T buffer[RING_BUFFER_MAX_SIZE]; // Fixed-size buffer
   int size;                       // Current size of the buffer
   int head;                       // Index of the next write
   int tail;                       // Index of the next read
public:
   RingBuffer();
   ~RingBuffer();

   int Push(const T &item);
   int Pop(T *item);
   int Get(int index, T *value);
   bool IsEmpty();
   bool IsFull();
};

#endif // _ring_buffer_h_
