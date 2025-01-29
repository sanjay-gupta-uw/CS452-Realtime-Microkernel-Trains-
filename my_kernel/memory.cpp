#include "memory.h"

#include "rpi.h"

MemoryManager::MemoryManager()
{
   uint32_t stack_addr = STACK_START + STACK_SIZE;
   MemoryBlock *last_block = nullptr;

   for (int i = 0; i < MBLOCK_COUNT; ++i)
   {
      free_blocks.Push(i);

      memory_blocks[i].index = i;
      memory_blocks[i].addr = stack_addr & ~0xF; // Align the stack pointer to 16 bytes
      memory_blocks[i].next = last_block;

      stack_addr += STACK_SIZE; // verify this
      last_block = &memory_blocks[i];
      // print range for memory block
      // uart_printf(CONSOLE, "Memory block %d: [0x%x - 0x%x)\n", i, memory_blocks[i].addr, memory_blocks[i].addr - STACK_SIZE);
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

void MemoryManager::Free(int block_index)
{
   free_blocks.Push(block_index);
}