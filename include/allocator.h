#ifndef MALLOC_ALLOCATOR_H
#define MALLOC_ALLOCATOR_H

#include <cstdint>
#include <memory>

#include "linked-list.h"

// 采用数组模拟heap空间，初始给予4KB(one page)空间
// heap's bytes range:
// [heap_start_vaddr, heap_end_vaddr) or [heap_start_vaddr, heap_end_vaddr - 1]
// [0,1,2,3] - unused(类比封面)
// [4,5,6,7,8,9,10,11] - prologue block(类比目录)
// [12, ..., 4096 * n - 5] - regular blocks(类比正文)
// 4096 * n + [- 4, -3, -2, -1] - epilogue block (header only)
extern uint64_t heap_start_vaddr;
extern uint64_t heap_end_vaddr;

// heap 区最多分配8个page，初始分配1个page
const uint64_t HEAP_MAX_SIZE = 4096 * 8;
extern uint8_t heap[];

const uint32_t FREE = 0;        // 空闲block
const uint32_t ALLOCATED = 1;   // 已分配的block
const uint64_t NIL = 0;         // 非法/空的虚拟地址

const uint32_t MIN_EXPLICIT_FREE_LIST_BLOCKSIZE = 16;
//const uint64_t MIN_REDBLACK_TREE_BLOCKSIZE = 24;  // rbt使用最小条件
const uint64_t MIN_REDBLACK_TREE_BLOCKSIZE = 40;    // 使用rbt管理 >= 40的块

// to allocate physical page for heap
uint32_t extend_heap(uint32_t size);
void os_syscall_brk();

// 将x向上对齐到n的整数倍
uint64_t round_up(uint64_t x, uint64_t n);

// operations for all blocks
uint32_t get_block_size(uint64_t header_vaddr);
void set_block_size(uint64_t header_vaddr, uint32_t blocksize);

uint32_t get_allocated(uint64_t header_vaddr);
void set_allocated(uint64_t header_vaddr, uint32_t allocated);

uint64_t get_payload(uint64_t vaddr);
uint64_t get_header(uint64_t vaddr);
uint64_t get_footer(uint64_t vaddr);

// operations for heap linked list
uint64_t get_next_header(uint64_t vaddr);
uint64_t get_prev_header(uint64_t vaddr);

uint64_t get_prologue();
uint64_t get_epilogue();

uint64_t get_first_block();
uint64_t get_last_block();

bool is_first_block(uint64_t vaddr);
bool is_last_block(uint64_t vaddr);

// for free block as data structure
uint64_t get_field32_block_ptr(uint64_t header_vaddr, uint32_t min_block_size, uint32_t offset);
bool set_field32_block_ptr(uint64_t header_vaddr, uint64_t block_ptr, uint32_t min_block_size, uint32_t offset);

// for debug
void print_heap();

// interface
bool heap_init();

uint64_t mem_alloc(uint32_t size);

void mem_free(uint64_t payload_vaddr);

#endif //MALLOC_ALLOCATOR_H
