#ifndef _memory_h_
#define _memory_h_

#include "ringbuffer.h"
#include <cstdint>

#define MBLOCK_SIZE 1024
#define MBLOCK_COUNT 64

// verify
#define TOP_OF_MEMORY 0x83E0C

struct MemoryBlock
{
   uint32_t addr;
   MemoryBlock *next;
   int index;
};

class MemoryManager
{
private:
   MemoryBlock memory_blocks[MBLOCK_COUNT];
   int arr[MBLOCK_COUNT]; // indices for free_blocks
   RingBuffer<int> free_blocks;

public:
   MemoryManager();
   ~MemoryManager();
   MemoryBlock *Allocate();
   void Free(MemoryBlock *block);
};

#endif // _memory_h_