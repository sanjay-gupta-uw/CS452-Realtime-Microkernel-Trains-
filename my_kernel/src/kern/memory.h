#ifndef _memory_h_
#define _memory_h_

#include "../containers/ringbuffer.h"
#include <cstdint>

#define KILOBYTE 1024
#define MEGABYTE 1024 * KILOBYTE

// WAY WITHIN 1 GB OF MEMORY
// 32 MB of memory --> goes from 0x80000 to 0x1F14800
#define STACK_SIZE 4 * MEGABYTE

#define MBLOCK_COUNT 64
// verify
#define STACK_START 0x90000

struct MemoryBlock
{
   uint64_t addr;
   MemoryBlock *next;
   int index;
};

class MemoryManager
{
private:
   MemoryBlock memory_blocks[MBLOCK_COUNT];
   RingBuffer<int> free_blocks;

public:
   MemoryManager();
   ~MemoryManager();
   MemoryBlock *Allocate();
   void Free(int block_index);
};

#endif // _memory_h_

// 589824
// 1114112
// 1638400
// 2162688