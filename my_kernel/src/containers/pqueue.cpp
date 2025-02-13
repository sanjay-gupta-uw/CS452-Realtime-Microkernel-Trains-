#include "pqueue.h"

// instantiate the template for task descriptor
// template class PQueue<TaskDescriptor *>;
template class PQueue<int>;

template <typename T>
PQueue<T>::PQueue()
{
   size = 0;
}

template <typename T>
PQueue<T>::~PQueue()
{
}

template <typename T>
int PQueue<T>::Push(T item, int priority)
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

template <typename T>
int PQueue<T>::Pop(T *item)
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

template <typename T>
int PQueue<T>::Peek(T *item) {
    if (isEmpty()) {
        return -1;  // Indicate that the queue is empty
    }
    *item = heap[0].data;  // Store the top item in the provided pointer
    return 0;  // Success
}


template <typename T>
bool PQueue<T>::isFull() const
{
   return size == HEAP_SIZE;
}

template <typename T>
bool PQueue<T>::isEmpty() const
{
   return size == 0;
}

template <typename T>
void PQueue<T>::heapifyUp(int index)
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

template <typename T>
void PQueue<T>::heapifyDown(int index)
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

template <typename T>
void PQueue<T>::swap(int index1, int index2)
{
   Node temp = heap[index1];
   heap[index1] = heap[index2];
   heap[index2] = temp;
}