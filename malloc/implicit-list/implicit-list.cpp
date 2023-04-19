#include <cassert>

#include "allocator.h"
#include "small-list.h"

/* ------------------------------------- */
/*  Implicit Free List                   */
/* ------------------------------------- */

/*  Allocated block:

    ff ff ff f9/f1  [8n + 24] - footer
    ?? ?? ?? ??     [8n + 20] - padding
    xx xx ?? ??     [8n + 16] - payload & padding
    xx xx xx xx     [8n + 12] - payload
    xx xx xx xx     [8n + 8] - payload
    hh hh hh h9/h1  [8n + 4] - header

    Free block:

    ff ff ff f8/f0  [8n + 24] - footer
    ?? ?? ?? ??     [8n + 20]
    ?? ?? ?? ??     [8n + 16]
    ?? ?? ?? ??     [8n + 12]
    ?? ?? ?? ??     [8n + 8]
    hh hh hh h8/h0  [8n + 4] - header
*/

const uint32_t MIN_IMPLICIT_FREE_LIST_BLOCK_SIZE = 8;

/* ------------------------------------- */
/*  Implementation                       */
/* ------------------------------------- */

bool implicit_list_initialize_free_block() {
    // init small block list
    small_list_init();

    return true;
}

uint64_t implicit_list_search_free_block(uint32_t payload_size, uint32_t &alloc_block_size) {
    // search 8-byte block list
    if (payload_size <= 4 && small_list->count() != 0) {
        // a small block and 8-byte is not empty
        alloc_block_size = 8;
        return small_list->head();
    }

    // payload size round up + header + footer
    uint32_t free_block_size = round_up(payload_size, 8) + 4 + 4;
    alloc_block_size = free_block_size;

    // search the whole heap
    // 从头开始遍历：首次适应算法
    uint64_t b = get_first_block();
    while (b <= get_last_block()) {
        uint32_t b_block_size = get_block_size(b);
        uint32_t b_allocated = get_allocated(b);

        if (b_allocated == FREE && b_block_size >= free_block_size) {
            return b;
        } else {
            b = get_next_header(b);
        }
    }

    return NIL;
}

bool implicit_list_insert_free_block(uint64_t free_header) {
    assert(free_header % 8 == 4);
    assert(get_first_block() <= free_header && free_header <= get_last_block());
    assert(get_allocated(free_header) == FREE);

    uint32_t block_size = get_block_size(free_header);
    assert(block_size % 8 == 0);
    assert(block_size >= 8);

    // 隐式空闲链表无需管理空闲块，对于8-Byte块需要使用small-list管理
    switch (block_size) {
        case 8:
            small_list_insert(free_header);
            break;
        default:
            break;
    }

    return true;
}

bool implicit_list_delete_free_block(uint64_t free_header) {
    assert(free_header % 8 == 4);
    assert(get_first_block() <= free_header && free_header <= get_last_block());
    assert(get_allocated(free_header) == FREE);

    uint32_t block_size = get_block_size(free_header);
    assert(block_size % 8 == 0);
    assert(block_size >= 8);

    // 隐式空闲链表无需管理空闲块，对于8-Byte块需要使用small-list管理
    switch (block_size) {
        case 8:
            small_list_delete(free_header);
            break;
        default:
            break;
    }

    return true;
}

void implicit_list_check_free_block() {
    small_list_check_free_blocks();
}
