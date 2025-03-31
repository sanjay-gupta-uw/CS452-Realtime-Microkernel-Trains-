#ifndef _STACK_H
#define _STACK_H

#include <cstddef>

template <typename T, std::size_t Capacity>
class Stack
{
private:
    T buffer[Capacity]; // Use the template parameter as the array size.
    std::size_t size;   // Current number of elements.
    std::size_t head;   // Index for next write.

public:
    Stack() : size(0), head(0) {}

    // Push an item to the stack. Returns 0 on success, -1 if the stack is full.
    int Push(T item)
    {
        if (size == Capacity)
            return -1; // Stack is full.

        buffer[head] = item;
        head = (head + 1) % Capacity;
        ++size;
        return 0;
    }

    // Pop an item from the stack. Returns 0 on success, -1 if the stack is empty.
    int Pop(T *item)
    {
        if (size == 0)
            return -1; // stack is empty.

        head = (head - 1) % Capacity;
        *item = buffer[head];
        --size;
        return 0;
    }

    // Check if the stack is empty.
    bool IsEmpty() const
    {
        return size == 0;
    }

    // Check if the stack is full.
    bool IsFull() const
    {
        return size == Capacity;
    }

    void Clear()
    {
        size = 0;
        head = 0;

        // Clear the stack by popping all items.
        while (!IsEmpty())
        {
            T item;
            Pop(&item);
        }
    }
};

#endif // _STACK_H
