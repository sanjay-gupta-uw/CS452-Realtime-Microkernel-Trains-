#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <cstddef>

template <typename T, std::size_t Capacity>
class Queue
{
private:
    T buffer[Capacity]; // Use the template parameter as the array size.
    std::size_t size;   // Current number of elements.
    std::size_t head;   // Index for next write.
    std::size_t tail;   // Index for next read.

public:
    Queue() : size(0), head(0), tail(0) {}

    // Push an item to the queue. Returns 0 on success, -1 if the queue is full.
    int Push(T item)
    {
        if (size == Capacity)
            return -1; // Queue is full.

        buffer[head] = item;
        head = (head + 1) % Capacity;
        ++size;
        return 0;
    }

    // Pop an item from the queue. Returns 0 on success, -1 if the queue is empty.
    int Pop(T *item)
    {
        if (size == 0)
            return -1; // Queue is empty.

        *item = buffer[tail];
        tail = (tail + 1) % Capacity;
        --size;
        return 0;
    }

    // Check if the queue is empty.
    bool IsEmpty() const
    {
        return size == 0;
    }

    // Check if the queue is full.
    bool IsFull() const
    {
        return size == Capacity;
    }
};

#endif // _QUEUE_H_
