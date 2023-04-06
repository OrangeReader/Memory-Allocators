#include "allocator.h"
#include <cassert>
#include <cstdio>

// 初始值不应该在此处设置(heap init中),下面设置只是为了通过单元测试
uint64_t heap_start_vaddr = 0;      // for pass uint-test
uint64_t heap_end_vaddr = 4096;     // for pass uint-test
uint8_t heap[HEAP_MAX_SIZE];

#ifdef DEBUG_MALLOC
char debug_message[1000];
#endif

/* ------------------------------------- */
/*  Operating System Implemented         */
/* ------------------------------------- */
void os_syscall_brk() {
    // an empty function
}

uint32_t extend_heap(uint32_t size) {
    // round up to page alignment
    size = (uint32_t) round_up((uint64_t)size, 4096);
    if (heap_end_vaddr - heap_start_vaddr + size <= HEAP_MAX_SIZE) {
        // do brk system call to request pages for heap
        os_syscall_brk();
        heap_end_vaddr += size;
    } else {
        return 0;
    }

    // 设置epilogue(尾言), head only
    uint64_t epilogue = get_epilogue();
    set_allocated(epilogue, ALLOCATED);
    set_block_size(epilogue, 0);

    return size;
}

/** 将x向上对齐到n的整数倍
 * if (x == k * n)
 * return x
 * else, x = k * n + m and m < n
 * return (k +1) * n
 */
uint64_t round_up(uint64_t x, uint64_t n) {
    // 一行简化版本
    return n * ((x + n - 1) / n);
}

/* ------------------------------------- */
/*  Block Operations                     */
/* ------------------------------------- */

// applicable for both header & footer
uint32_t get_block_size(uint64_t header_vaddr) {
    if (header_vaddr == NIL) {
        return 0;
    }

    // 地址合法性判断
    assert(get_prologue() <= header_vaddr && header_vaddr <= get_epilogue());
    // header & footer should be 4 bytes alignment
    assert((header_vaddr & 0x3) == 0x0);

    uint32_t header_value = *(uint32_t *)&heap[header_vaddr];
    return (header_value & 0xFFFFFFF8);
}

// applicable for both header & footer
void set_block_size(uint64_t header_vaddr, uint32_t blocksize) {
    if (header_vaddr == NIL) {
        return;
    }

    assert(get_prologue() <= header_vaddr && header_vaddr <= get_epilogue());
    assert((header_vaddr & 0x3) == 0x0);  // header & footer should be 4 bytes alignment
    assert((blocksize & 0x7) == 0x0);   // block size should be 8 bytes aligned

    // 清除原来的size信息,但保持allocated信息
    *(uint32_t *)&heap[header_vaddr] &= 0x00000007;
    *(uint32_t *)&heap[header_vaddr] |= blocksize;  // 设置新的size
}

// applicable for both header & footer
uint32_t get_allocated(uint64_t header_vaddr) {
    if(header_vaddr == NIL) {
        // NULL can be considered as allocated
        // 这样合并的时候可以不用考虑前面的block，这也是合理的
        // 但基本不可能走到这个条件中，因为我们的heap结构
        return ALLOCATED;
    }

    assert(get_prologue() <= header_vaddr && header_vaddr <= get_epilogue());
    assert((header_vaddr & 0x3) == 0x0);  // header & footer should be 4 bytes alignment

    uint32_t header_value = *(uint32_t *)&heap[header_vaddr];
    return (header_value & 0x1);
}

// applicable for both header & footer
void set_allocated(uint64_t header_vaddr, uint32_t allocated) {
    if (header_vaddr == NIL) {
        return;
    }

    assert(get_prologue() <= header_vaddr && header_vaddr <= get_epilogue());
    assert((header_vaddr & 0x3) == 0x0);

    // 清除allocated信息，但是保持size信息
    *(uint32_t *)(&heap[header_vaddr]) &= 0xFFFFFFF8;
    // 设置allocated信息，同时确保allocated只有一位
    *(uint32_t *)(&heap[header_vaddr]) |= (allocated & 0x1);
    return ;
}

uint64_t get_payload(uint64_t vaddr) {
    if (vaddr == NIL) {
        return NIL;
    }

    assert(get_first_block() <= vaddr && vaddr < get_epilogue());

    // vaddr can be:
    // 1. starting address of the block (8 * n + 4)
    // 2. starting address of the payload (8 * m)
    assert((vaddr & 0x3) == 0);

    return round_up(vaddr, 8);
}

uint64_t get_header(uint64_t vaddr) {
    if (vaddr == NIL) {
        return NIL;
    }

    assert(get_first_block() <= vaddr && vaddr < get_epilogue());

    // vaddr can be:
    // 1. starting address of the block (8 * n + 4)
    // 2. starting address of the payload (8 * m)
    assert((vaddr & 0x3) == 0);

    // 上述条件保证了到此处的vaddr不是NIL，是一个合法地址
    return get_payload(vaddr) - 4;
}

uint64_t get_footer(uint64_t vaddr) {
    if (vaddr == NIL) {
        return NIL;
    }

    assert(get_first_block() <= vaddr && vaddr < get_epilogue());

    // vaddr can be:
    // 1. starting address of the block (8 * n + 4)
    // 2. starting address of the payload (8 * m)
    assert((vaddr & 0x3) == 0);

    uint64_t header_vaddr = get_header(vaddr);
    uint64_t footer_vaddr = header_vaddr + get_block_size(header_vaddr) - 4;

    assert(get_first_block() < footer_vaddr && footer_vaddr < get_epilogue());
    assert(get_first_block() < footer_vaddr && footer_vaddr < get_epilogue());
    return footer_vaddr;
}

/* ------------------------------------- */
/*  Heap Operations                      */
/* ------------------------------------- */
uint64_t get_next_header(uint64_t vaddr) {
    if (vaddr == NIL || is_last_block(vaddr)) {
        // 对于空/非法地址 和 最后一块block, 返回 NIL, 标记为非法
        return NIL;
    }

    assert(get_first_block() <= vaddr && vaddr < get_epilogue());

    // vaddr can be:
    // 1. starting address of the block (8 * n + 4)
    // 2. starting address of the payload (8 * m)
    assert((vaddr & 0x3) == 0);
    uint64_t header_vaddr = get_header(vaddr);
    uint64_t block_size = get_block_size(header_vaddr);

    uint64_t next_header_vaddr = header_vaddr + block_size;
    assert(get_first_block() < next_header_vaddr && next_header_vaddr <= get_last_block());

    return next_header_vaddr;
}

uint64_t get_prev_header(uint64_t vaddr) {
    if (vaddr == NIL || is_first_block(vaddr)) {
        // 对于空/非法地址 和 第一块block, 返回 NIL, 标记为非法
        return NIL;
    }

    assert(get_first_block() <= vaddr && vaddr < get_epilogue());

    // vaddr can be:
    // 1. starting address of the block (8 * n + 4)
    // 2. starting address of the payload (8 * m)
    assert((vaddr & 0x3) == 0);
    uint64_t header_vaddr = get_header(vaddr);

    uint64_t prev_footer_vaddr = header_vaddr - 4;
    uint64_t prev_block_size = get_block_size(prev_footer_vaddr);

    uint64_t prev_header_vaddr = header_vaddr - prev_block_size;

    assert(get_first_block() <= prev_header_vaddr && prev_header_vaddr < get_last_block());
    assert(get_block_size(prev_header_vaddr) == prev_block_size);
    assert(get_allocated(prev_header_vaddr) == get_allocated(prev_footer_vaddr));

    return prev_header_vaddr;
}

uint64_t get_prologue() {
    assert(heap_end_vaddr > heap_start_vaddr);
    assert((heap_end_vaddr - heap_start_vaddr) % 4096 == 0);
    assert(heap_start_vaddr % 4096 == 0);

    // 4 for the not in use
    return heap_start_vaddr + 4;
}

uint64_t get_epilogue() {
    assert(heap_end_vaddr > heap_start_vaddr);
    assert((heap_end_vaddr - heap_start_vaddr) % 4096 == 0);
    assert(heap_start_vaddr % 4096 == 0);

    // epilogue block is having header only
    return heap_end_vaddr - 4;
}

uint64_t get_first_block() {
    assert(heap_end_vaddr > heap_start_vaddr);
    assert((heap_end_vaddr - heap_start_vaddr) % 4096 == 0);
    assert(heap_start_vaddr % 4096 == 0);

    // 4 for the not in use
    // 8 for the prologue block
    return get_prologue() + 8;
}

uint64_t get_last_block() {
    assert(heap_end_vaddr > heap_start_vaddr);
    assert((heap_end_vaddr - heap_start_vaddr) % 4096 == 0);
    assert(heap_start_vaddr % 4096 == 0);

    uint64_t epilogue_header = get_epilogue();
    uint64_t last_footer = epilogue_header - 4;
    uint32_t last_block_size = get_block_size(last_footer);

    uint64_t last_header = epilogue_header - last_block_size;
    assert(get_first_block() <= last_header);

    return last_header;

}

bool is_first_block(uint64_t vaddr) {
    if (vaddr == NIL) {
        return false;
    }

    // vaddr can be:
    // 1. starting address of the block (8 * n + 4)
    // 2. starting address of the payload (8 * m)
    assert(get_first_block() <= vaddr && vaddr < get_epilogue());
    assert((vaddr & 0x3) == 0x0);

    uint64_t header_vaddr = get_header(vaddr);
    if (header_vaddr == get_first_block()) {
        return true;
    }
    return false;
}

bool is_last_block(uint64_t vaddr) {
    if (vaddr == NIL) {
        return false;
    }

    // vaddr can be:
    // 1. starting address of the block (8 * n + 4)
    // 2. starting address of the payload (8 * m)
    assert(get_first_block() <= vaddr && vaddr < get_epilogue());
    assert((vaddr & 0x3) == 0x0);

    uint64_t header_vaddr = get_header(vaddr);
    uint32_t block_size = get_block_size(header_vaddr);

    if (header_vaddr + block_size == get_epilogue()) {
        return true;
    }
    return false;
}

/* ------------------------------------- */
/*  Free Block as Data Structure         */
/* ------------------------------------- */
uint64_t get_field32_block_ptr(uint64_t header_vaddr, uint32_t min_block_size, uint32_t offset) {
    if (header_vaddr == NIL) {
        return NIL;
    }

    assert(get_first_block() <= header_vaddr && header_vaddr <= get_last_block());
    assert(header_vaddr % 8 == 4);
    assert(get_block_size(header_vaddr) >= min_block_size);

    assert(offset % 4 == 0);
    uint32_t vaddr_32 = *(uint32_t *)&heap[header_vaddr + offset];
    return (uint64_t)vaddr_32;
}

bool set_field32_block_ptr(uint64_t header_vaddr, uint64_t block_ptr, uint32_t min_block_size, uint32_t offset) {
    if (header_vaddr == NIL) {
        return false;
    }

    assert(get_first_block() <= header_vaddr && header_vaddr <= get_last_block());
    assert(header_vaddr % 8 == 4);
    assert(get_block_size(header_vaddr) >= min_block_size);

    assert(block_ptr == NIL || (get_first_block() <= block_ptr && block_ptr <= get_last_block()));
    assert(block_ptr == NIL || (block_ptr % 8 == 4));
    assert(block_ptr == NIL || (get_block_size(block_ptr) >= min_block_size));

    assert(offset % 4 == 0);

    // actually is a 32-bit pointer
    assert((block_ptr >> 32) == 0);
    *(uint32_t *)&heap[header_vaddr + offset] = (uint32_t)(block_ptr & 0xFFFFFFFF);

    return true;
}


/* ------------------------------------- */
/*  Malloc and Free                      */
/* ------------------------------------- */
uint64_t merge_blocks_as_free(uint64_t low, uint64_t high) {
    assert(low % 8 == 4);
    assert(high % 8 == 4);
    assert(get_first_block() <= low && low < get_last_block());
    assert(get_first_block() < high && high <= get_last_block());
    assert(get_next_header(low) == high);
    assert(get_prev_header(high) == low);

    // must merge as free
    uint32_t block_size = get_block_size(low) + get_block_size(high);

    set_block_size(low, block_size);
    set_allocated(low, FREE);

    uint64_t footer = get_footer(high);
    set_block_size(footer, block_size);
    set_allocated(footer, FREE);

    return low;
}

// 首次适配内存分配算法：找到第一块合适的block用于分配
uint64_t try_alloc_with_splitting(uint64_t block_vaddr, uint32_t request_block_size, uint32_t min_block_size) {
    if (request_block_size < min_block_size) {
        return NIL;
    }

    uint64_t b = block_vaddr;
    uint32_t b_block_size = get_block_size(b);
    uint32_t b_allocated = get_allocated(b);

    if (b_allocated == FREE && b_block_size >= request_block_size) {
        // allocated this block
        if (b_block_size - request_block_size >= min_block_size) {
            // split this block `b`
            uint64_t next_footer = get_footer(b);
            set_allocated(next_footer, FREE);
            set_block_size(next_footer, b_block_size - request_block_size);

            set_allocated(b, ALLOCATED);
            set_block_size(b, request_block_size);

            uint64_t b_footer = get_footer(b);
            set_allocated(b_footer, ALLOCATED);
            set_block_size(b_footer, request_block_size);

            // set the left splitted block
            // in the extreme situation, next block size == 8
            // which makes the whole block of next to be:
            // [0x00000008, 0x00000008]
            uint64_t next_header = get_next_header(b);
            set_allocated(next_header, FREE);
            set_block_size(next_header, b_block_size - request_block_size);

            assert(get_footer(next_header) == next_footer);

            return get_payload(b);
        } else {
            // 剩余的空间已经无法满足设置一个空闲块了
            return NIL;
        }
    }

    // 当前空间无法满足分配或是已分配
    return NIL;
}

uint64_t try_extend_heap_to_alloc(uint32_t size, uint32_t min_block_size) {
    // get the size to be added
    uint64_t old_last = get_last_block();

    uint32_t last_allocated = get_allocated(old_last);
    uint32_t last_block_size = get_block_size(old_last);

    uint32_t to_request_from_OS = size;
    if (last_allocated == FREE) {
        // last block can help the request
        to_request_from_OS -= last_block_size;
    }

    uint64_t old_epilogue = get_epilogue();

    uint32_t os_allocated_size = extend_heap(to_request_from_OS);
    if (os_allocated_size) {
        assert(os_allocated_size >= 4096);
        assert(os_allocated_size % 4096 == 0);

        uint64_t block_header = NIL;

        // now last block is different
        // but we check the old last block
        if (last_allocated == ALLOCATED) {
            // no merging is needed
            // take place the old epilogue as new last
            uint64_t new_last = old_epilogue;
            set_allocated(new_last, FREE);
            set_block_size(new_last, os_allocated_size);

            uint64_t new_last_footer = get_footer(new_last);
            set_allocated(new_last_footer, FREE);
            set_block_size(new_last_footer, os_allocated_size);

            block_header = new_last;
        } else {
            // merging with last_block is needed
            set_allocated(old_last, FREE);
            set_block_size(old_last, last_block_size + os_allocated_size);

            uint64_t last_footer = get_footer(old_last);
            set_allocated(last_footer, FREE);
            set_block_size(last_footer, last_block_size + os_allocated_size);

            block_header = old_last;
        }

        // try to allocate
        uint64_t payload_vaddr = try_alloc_with_splitting(block_header, size, min_block_size);
        if (payload_vaddr != NIL)
        {
#ifdef DEBUG_MALLOC
            check_heap_correctness();
#endif
            return payload_vaddr;
        }
    }

    // else, no page can be allocated
#ifdef DEBUG_MALLOC
    check_heap_correctness();
    printf("OS cannot allocate physical page for heap!\n");
#endif

    return NIL;
}

/* ------------------------------------- */
/*  Debugging and Correctness Checking   */
/* ------------------------------------- */
void check_heap_correctness() {
    int linear_free_counter = 0;
    uint64_t p = get_first_block();
    while(p != NIL && p <= get_last_block()) {
        assert(p % 8 == 4);
        assert(get_first_block() <= p && p <= get_last_block());

        uint64_t f = get_footer(p);
//        printf("header = %lu, size = %lu\n", p, get_block_size(p));
//        printf("footer = %lu, size = %lu\n", p, get_block_size(f));

        assert(get_block_size(p) == get_block_size(f));
        assert(get_allocated(p) == get_allocated(f));

        // rule 1: block[0] ==> A/F
        // rule 2: block[-1] ==> A/F
        // rule 3: block[i] == A ==> block[i-1] == A/F && block[i+1] == A/F
        // rule 4: block[i] == F ==> block[i-1] == A && block[i+1] == A
        // these 4 rules ensures that
        // adjacent free blocks are always merged together
        // henceforth external fragmentation are minimized
        if (get_allocated(p) == FREE) {
            linear_free_counter += 1;
        } else {
            linear_free_counter = 0;
        }

        assert(linear_free_counter <= 1);

        p = get_next_header(p);
    }
}

void print_heap() {
    printf("============\nheap blocks:\n");
    uint64_t h = get_first_block();
    int i = 0;
    while (h != NIL && h < get_epilogue()) {
        uint32_t a = get_allocated(h);
        uint32_t s = get_block_size(h);
        uint64_t f = get_footer(h);

        printf("[H:%lu,F:%lu,S:%u,A:%u]  ", h, f, s, a);
        h = get_next_header(h);

        i += 1;
        if (i % 5 == 0) {
            printf("\b\n");
        }
    }
    printf("\b\b\n============\n");
}