#ifndef _pqueue_h_
#define _pqueue_h_

#define HEAP_SIZE 100

template <typename T, std::size_t Capacity>
class PQueue
{
private:
    struct Node
    {
        T data;
        int priority;
    };

    Node heap[Capacity]; // Min-heap since 0 is highest priority
    int size;

    void heapifyUp(int index) // Restore heap property going up
    {
        while (index > 0)
        {
            int parent = (index - 1) / 2;
            if (heap[index].priority < heap[parent].priority)
            {
                swap(index, parent);
                index = parent;
            }
            else
            {
                break;
            }
        }
    }
    void heapifyDown(int index) // Restore heap property going down
    {
        while (true)
        {
            int left = 2 * index + 1;
            int right = 2 * index + 2;
            int highest_priority = index;

            if (left < size && heap[left].priority < heap[index].priority)
            {
                highest_priority = left;
            }
            if (right < size && heap[right].priority < heap[highest_priority].priority)
            {
                highest_priority = right;
            }

            if (highest_priority == index)
            {
                break;
            }

            swap(index, highest_priority);
            index = highest_priority;
        }
    }
    void swap(int index1, int index2)
    {
        Node temp = heap[index1];
        heap[index1] = heap[index2];
        heap[index2] = temp;
    }

public:
    PQueue() : size(0) {}
    ~PQueue() {}

    int Push(T item, int priority)
    {
        if (isFull())
        {
            return -1;
        }

        int index = size++;
        heap[index].data = item;
        heap[index].priority = priority;

        heapifyUp(index);

        return 0;
    }
    int Pop(T *item)
    {
        if (isEmpty())
        {
            return -1;
        }

        *item = heap[0].data;
        heap[0] = heap[--size];
        heapifyDown(0);

        return 0;
    }

    int Peek(T *item, int *priority)
    {
        if (isEmpty())
        {
            item = nullptr;
            return -1; // Indicate that the queue is empty
        }
        *item = heap[0].data; // Store the top item in the provided pointer
        *priority = heap[0].priority;
        return 0; // Success
    }

    bool isEmpty() const
    {
        return size == 0;
    }
    bool isFull() const
    {
        return size == HEAP_SIZE;
    }

    void Clear()
    {
        while (!isEmpty())
        {
            T item;
            Pop(&item);
        }
        size = 0; // Reset size to 0
    }
};

#endif // _pqueue_h_