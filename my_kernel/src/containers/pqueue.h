#ifndef _pqueue_h_
#define _pqueue_h_

#define HEAP_SIZE 100

template <typename T>
class PQueue
{
public:
   PQueue();
   ~PQueue();

   int Push(T item, int priority);
   int Pop(T *item);
   bool isEmpty() const;
   bool isFull() const;

private:
   struct Node
   {
      T data;
      int priority;
   };

   Node heap[HEAP_SIZE]; // Min-heap since 0 is highest priority
   int size;

   void heapifyUp(int index);   // Restore heap property going up
   void heapifyDown(int index); // Restore heap property going down
   void swap(int index1, int index2);
};

#endif // _pqueue_h_