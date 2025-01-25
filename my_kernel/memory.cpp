#include "memory.h"

MemoryManager::MemoryManager()
{
   uint32_t current_addr = TOP_OF_MEMORY;
   MemoryBlock *last_block = nullptr;

   for (int i = 0; i < MBLOCK_COUNT; ++i)
   {
      arr[i] = i;
      free_blocks.Push(&arr[i]);

      memory_blocks[i].index = i;
      memory_blocks[i].addr = current_addr;
      memory_blocks[i].next = last_block;

      current_addr += MBLOCK_SIZE; // verify this
      last_block = &memory_blocks[i];
   }
}

MemoryManager::~MemoryManager()
{
}

MemoryBlock *MemoryManager::Allocate()
{
   int block_id;
   if (free_blocks.Pop(&block_id) == -1)
   {
      return nullptr;
   }

   return &memory_blocks[block_id];
}

void MemoryManager::Free(MemoryBlock *block)
{
   free_blocks.Push(&arr[block->index]);
}