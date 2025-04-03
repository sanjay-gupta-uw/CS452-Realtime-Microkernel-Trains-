#ifndef _STACK_H
#define _STACK_H

#include <cstddef>
#include "../user/include/track_node.h"

template <typename T, std::size_t Capacity>
class Stack
{
public:
    std::size_t size; // Current number of elements.
private:
    T buffer[Capacity]; // Use the template parameter as the array size.
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

        head = (head + Capacity - 1) % Capacity;

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

    void Print()
    {
        IO_NS::PrintTerminal(COLOR_MAGENTA "Stack contents: {size: %d, head: %d} ", size, head);
        if (IsEmpty())
        {
            IO_NS::PrintTerminal("\r\n");
            return;
        }
        Stack<T, Capacity> temp;

        while (!IsEmpty())
        {
            T item;
            Pop(&item);

            // cast item to PathNode
            PathNode *node = &item; // No cast needed
            IO_NS::PrintTerminal(COLOR_MAGENTA "%s ", node->node->name);
            temp.Push(item);
        }
        while (!temp.IsEmpty())
        {
            T item;
            temp.Pop(&item);
            Push(item);
        }
        IO_NS::PrintTerminal("\r\n");
    }

    void
    Clear()
    {
        // Clear the stack by popping all items.
        while (!IsEmpty())
        {
            T item;
            Pop(&item);
        }
        uassert(size == 0 && "Stack::Clear -- Stack not cleared");
    }
};

#endif // _STACK_H
