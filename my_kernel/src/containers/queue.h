#ifndef _queue_h_
#define _queue_h_

#define QUEUE_MAX_SIZE 32

template <typename T>
class Queue
{
private:
    T buffer[QUEUE_MAX_SIZE]; // Fixed-size buffer
    int size;                 // Current size of the buffer
    int head;                 // Index of the next write
    int tail;                 // Index of the next read
public:
    Queue();
    ~Queue();

    int Push(T item);
    int Pop(T *item);
    bool IsEmpty();
    bool IsFull();
};

template <typename T>
Queue<T>::Queue() : size(0), head(0), tail(0) {}

template <typename T>
Queue<T>::~Queue() {}

// Push an item to the buffer: 0 if success, -1 if buffer is full
template <typename T>
int Queue<T>::Push(T item)
{
    if (size == QUEUE_MAX_SIZE)
    {
        // Buffer is full
        return -1;
    }

    buffer[head] = item;                // Copy the value into the buffer
    head = (head + 1) % QUEUE_MAX_SIZE; // Advance the head (circular)
    size++;                             // Increase the size

    return 0; // Success
}

// Pop an item from the buffer: 0 if success, -1 if buffer is empty
template <typename T>
int Queue<T>::Pop(T *item)
{
    if (size == 0)
    {
        // Buffer is empty
        return -1;
    }

    *item = buffer[tail];

    tail = (tail + 1) % QUEUE_MAX_SIZE; // Advance the tail (circular)
    size--;                             // Decrease the size

    return 0; // Success
}

// Check if the buffer is empty
template <typename T>
bool Queue<T>::IsEmpty()
{
    return size == 0;
}

// Check if the buffer is full
template <typename T>
bool Queue<T>::IsFull()
{
    return size == QUEUE_MAX_SIZE;
}

#endif // _queue_h_
