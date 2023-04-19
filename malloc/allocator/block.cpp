#include <cassert>

#include "allocator.h"

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

const int AF_BIT = 0;   // allocated / free bit
const int P8_BIT = 1;   // prev is 8 byte block bit
const int B8_BIT = 2;   // block is 8 byte bit

static void set_bit(uint64_t vaddr, int bit_offset) {
    uint64_t vector = 1 << bit_offset;

    assert((vaddr & 0x3) == 0x0);  // vaddr should be 4 bytes alignment
    assert(get_prologue() <= vaddr && vaddr <= get_epilogue());

    *reinterpret_cast<uint32_t *>(&heap[vaddr]) |= vector;
}

static void reset_bit(uint64_t vaddr, int bit_offset) {
    uint32_t vector = 1 << bit_offset;

    assert((vaddr & 0x3) == 0x0);  // vaddr should be 4 bytes alignment
    assert(get_prologue() <= vaddr && vaddr <= get_epilogue());

    *reinterpret_cast<uint32_t *>(&heap[vaddr]) &= (~vector);
}

// is_bit_set传入的是block vaddr
static bool is_bit_set(uint64_t vaddr, int bit_offset) {
    assert((vaddr & 0x3) == 0x0);  // vaddr should be 4 bytes alignment
    assert(get_prologue() <= vaddr && vaddr <= get_epilogue());

    return (*reinterpret_cast<uint32_t *>(&heap[vaddr]) >> bit_offset) & 0x1;
}

static void check_block8_correctness(uint64_t vaddr) {
    if (vaddr == NIL) {
        return;
    }

    assert(vaddr % 4 == 0);
    assert(get_prologue() <= vaddr && vaddr <= get_epilogue());

    if (vaddr % 8 == 4) {
        // header
        assert(is_bit_set(vaddr, B8_BIT));

        uint64_t next_header = vaddr + 8;

        // B8 cannot be epilogue
        assert(next_header <= get_epilogue());
        assert(is_bit_set(next_header, P8_BIT));

        if (get_allocated(vaddr) == ALLOCATED) {
            // size == 8
            assert((*reinterpret_cast<uint32_t *>(&heap[vaddr]) & 0xFFFFFFF8) == 8);
        }
    } else if (vaddr % 8 == 0) {
        // payload(footer)
        uint64_t next_header = vaddr + 8;

        // B8 cannot be epilogue
        assert(next_header <= get_epilogue());
        assert(is_bit_set(next_header, P8_BIT));

        vaddr -= 4;
        assert(is_bit_set(vaddr, B8_BIT));

        if (get_allocated(vaddr) == ALLOCATED) {
            // size == 8
            assert((*reinterpret_cast<uint32_t *>(&heap[vaddr]) & 0xFFFFFFF8) == 8);
        }
    } else {
        assert(false);
    }
}

static bool is_block8(uint64_t vaddr) {
    if (vaddr == NIL) {
        return false;
    }

    assert(get_prologue() <= vaddr && vaddr < get_epilogue());

    if (vaddr % 8 == 4) {
        // header
        if (is_bit_set(vaddr, B8_BIT)) {
#ifdef DEBUG_MALLOC
            check_block8_correctness(vaddr);
#endif
            return true;
        }
    } else if (vaddr % 8 == 0) {
        // payload(footer)
        uint64_t next_header = vaddr + 4;
        if (is_bit_set(next_header, P8_BIT)) {
#ifdef DEBUG_MALLOC
            check_block8_correctness(vaddr - 4);
#endif
            return true;
        }
    }
    return 0;
}

// applicable for both header & footer
uint32_t get_block_size(uint64_t header_vaddr) {
    if (header_vaddr == NIL) {
        return 0;
    }

    // 地址合法性判断
    assert(get_prologue() <= header_vaddr && header_vaddr <= get_epilogue());
    // header & footer should be 4 bytes alignment
    assert((header_vaddr & 0x3) == 0x0);

    if (is_block8(header_vaddr) == 1) {
#ifdef DEBUG_MALLOC
        check_block8_correctness(header_vaddr);
#endif
        return 8;
    } else {
        // B8 is unset - an ordinary block
        uint32_t header_value = *(uint32_t *)&heap[header_vaddr];
        return (header_value & 0xFFFFFFF8);
    }
}

// applicable for both header & footer
void set_block_size(uint64_t header_vaddr, uint32_t block_size) {
    if (header_vaddr == NIL) {
        return;
    }

    assert(get_prologue() <= header_vaddr && header_vaddr <= get_epilogue());
    assert((header_vaddr & 0x3) == 0x0);  // header & footer should be 4 bytes alignment
    assert((block_size & 0x7) == 0x0);   // block size should be 8 bytes aligned

    uint64_t next_header_vaddr;
    if (block_size == 8) {
        // small block is special
        if (header_vaddr % 8 == 0) {
            // do not set footer of small block
            // reset to header
            header_vaddr = header_vaddr - 4;
        }
        next_header_vaddr = header_vaddr + 8;

        set_bit(header_vaddr, B8_BIT);
        if (next_header_vaddr <= get_epilogue()) {
            set_bit(next_header_vaddr, P8_BIT);
        }

        if (get_allocated(header_vaddr) == FREE) {
            // free 8-byte does not set block size
            return;
        }
        // else, set header block size 8
    } else {
        // an ordinary block
        if (header_vaddr % 8 == 4) {
            // header
            next_header_vaddr = header_vaddr + block_size;
        } else {
            // footer
            next_header_vaddr = header_vaddr + 4;
        }

        reset_bit(header_vaddr, B8_BIT);
        if (next_header_vaddr <= get_epilogue()) {
            reset_bit(next_header_vaddr, P8_BIT);
        }
    }

    // 清除原来的size信息,但保持allocated信息
    *(uint32_t *)&heap[header_vaddr] &= 0x00000007;
    *(uint32_t *)&heap[header_vaddr] |= block_size;  // 设置新的size

#ifdef DEBUG_MALLOC
    if (block_size == 8) {
        check_block8_correctness(header_vaddr);
    }
#endif
}

// applicable for both header & footer for ordinary blocks
// header only for small block 8-Byte
uint32_t get_allocated(uint64_t header_vaddr) {
    if(header_vaddr == NIL) {
        // NULL can be considered as allocated
        // 这样合并的时候可以不用考虑前面的block，这也是合理的
        // 但基本不可能走到这个条件中，因为我们的heap结构
        return ALLOCATED;
    }

    assert(get_prologue() <= header_vaddr && header_vaddr <= get_epilogue());
    assert((header_vaddr & 0x3) == 0x0);  // header & footer should be 4 bytes alignment

    // ⭐ 如果传入的是footer，需要判断下这个是否是8 Byte block -> 通过+4获取next_header，其P8标识了
    // 如果是8 Byte block，则无需设置，设置header即可
    if (header_vaddr % 8 == 0) {
        // footer
        // check if 8-byte small block
        uint64_t next_header_vaddr = header_vaddr + 4;
        if (next_header_vaddr <= get_epilogue()) {
            // check P8 bit of next
            if (is_bit_set(next_header_vaddr, P8_BIT)) {
                // current block is 8-byte, no footer. check header instead
                header_vaddr -= 4;
#ifdef DEBUG_MALLOC
                check_block8_correctness(header_vaddr);
#endif
            }
            // else, current block has footer
        } else {
            // this is block is epilogue but it's 8X
            assert(false);
        }
    }

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

    if (header_vaddr % 8 == 0) {
        // footer
        // check if 8-byte small block
        uint64_t next_header_vaddr = header_vaddr + 4;
        if (next_header_vaddr <= get_epilogue()) {
            // check P8 bit of next
            if (is_bit_set(next_header_vaddr, P8_BIT)) {
                // current block is 8-byte, no footer. check header instead
                header_vaddr -= 4;
#ifdef DEBUG_MALLOC
                check_block8_correctness(header_vaddr);
#endif
            }
            // else, current block has footer
        } else {
            // this is block is epilogue but it's 8X
            assert(false);
        }
    }

    // 清除allocated信息，但是保持size + B8 + P8信息
    *(uint32_t *)(&heap[header_vaddr]) &= 0xFFFFFFFE;
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

// both for header and payload
uint64_t get_header(uint64_t vaddr) {
    if (vaddr == NIL) {
        return NIL;
    }

    assert(get_first_block() <= vaddr && vaddr <= get_epilogue());

    // vaddr can be:
    // 1. starting address of the block (8 * n + 4)
    // 2. starting address of the payload (8 * m)
    assert((vaddr & 0x3) == 0);

    // 上述条件保证了到此处的vaddr不是NIL，是一个合法地址
    return round_up(vaddr, 8) - 4;
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
    if (vaddr == NIL || vaddr == get_epilogue()) {
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
    assert(get_first_block() < next_header_vaddr && next_header_vaddr <= get_epilogue());

    return next_header_vaddr;
}

uint64_t get_prev_header(uint64_t vaddr) {
    if (vaddr == NIL || vaddr == get_prologue()) {
        // 对于空/非法地址 和 第一块block, 返回 NIL, 标记为非法
        return NIL;
    }

    assert(get_first_block() <= vaddr && vaddr <= get_epilogue());

    // vaddr can be:
    // 1. starting address of the block (8 * n + 4)
    // 2. starting address of the payload (8 * m)
    assert((vaddr & 0x3) == 0);
    uint64_t header_vaddr = get_header(vaddr);

    uint64_t prev_header_vaddr = NIL;

    // check P8 bit 0010
    if (is_bit_set(header_vaddr, P8_BIT) == 1) {
        // previous block is 8-byte block
        prev_header_vaddr = header_vaddr - 8;
#ifdef DEBUG_MALLOC
        check_block8_correctness(prev_header_vaddr);
#endif
        return prev_header_vaddr;
    } else {
        // previous block is bigger than 8 bytes
        uint64_t prev_footer_vaddr = header_vaddr - 4;
        uint64_t prev_block_size = get_block_size(prev_footer_vaddr);

        prev_header_vaddr = header_vaddr - prev_block_size;

        assert(get_first_block() <= prev_header_vaddr && prev_header_vaddr < get_epilogue());
        assert(get_block_size(prev_header_vaddr) == get_block_size(prev_footer_vaddr));
        assert(get_allocated(prev_header_vaddr) == get_allocated(prev_footer_vaddr));

        return prev_header_vaddr;
    }
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
    return get_prev_header(epilogue_header);
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