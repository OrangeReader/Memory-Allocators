#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "allocator.h"
#include "explicit-list.h"
#include "small-list.h"

/* ------------------------------------- */
/*  Explicit Free List                   */
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
    nn nn nn nn     [8n + 12] - next free block address
    pp pp pp pp     [8n + 8] - previous free block address
    hh hh hh h8/h0  [8n + 4] - header
*/

/* ------------------------------------- */
/*  Operations for List Block Structure  */
/* ------------------------------------- */
uint64_t EXPLICIT_FREE_LINKED_LIST::get_head() const {
    return head_;
}

bool EXPLICIT_FREE_LINKED_LIST::set_head(uint64_t new_head) {
    head_ = new_head;
    return true;
}

uint64_t EXPLICIT_FREE_LINKED_LIST::get_count() const {
    return count_;
}

bool EXPLICIT_FREE_LINKED_LIST::set_count(uint64_t new_count) {
    count_ = new_count;
    return true;
}

bool EXPLICIT_FREE_LINKED_LIST::destruct_node(uint64_t node) {
    // 没有额外分配内存，通过数组模拟链表，因此无需销毁
    return true;
}

bool EXPLICIT_FREE_LINKED_LIST::is_nodes_equal(uint64_t first, uint64_t second) {
    return first == second;
}

// prev 和 next 在空闲块中占 4 Byte(32 bits)
//  - prev 处于 块起始位置 偏移 4 Byte的位置
//  - next 处于 块起始位置 偏移 8 Byte的位置
//
//  footer
//  ...
//  next
//  prev
//  header
uint64_t EXPLICIT_FREE_LINKED_LIST::get_node_prev(uint64_t header_vaddr) {
    return get_field32_block_ptr(header_vaddr, MIN_EXPLICIT_FREE_LIST_BLOCKSIZE, 4);
}

bool EXPLICIT_FREE_LINKED_LIST::set_node_prev(uint64_t header_vaddr, uint64_t prev_vaddr) {
    return set_field32_block_ptr(header_vaddr, prev_vaddr, MIN_EXPLICIT_FREE_LIST_BLOCKSIZE, 4);
}

uint64_t EXPLICIT_FREE_LINKED_LIST::get_node_next(uint64_t header_vaddr) {
    return get_field32_block_ptr(header_vaddr, MIN_EXPLICIT_FREE_LIST_BLOCKSIZE, 8);
}

bool EXPLICIT_FREE_LINKED_LIST::set_node_next(uint64_t header_vaddr, uint64_t next_vaddr) {
    return set_field32_block_ptr(header_vaddr, next_vaddr, MIN_EXPLICIT_FREE_LIST_BLOCKSIZE, 8);
}

// The explicit free linked list
static std::shared_ptr<EXPLICIT_FREE_LINKED_LIST> explicit_list;

uint64_t explicit_list_search(uint32_t free_block_size) {
    // search explicit free list
    uint64_t b = explicit_list->head();
    uint32_t counter_copy = explicit_list->count();
    for (int i = 0; i < counter_copy; ++i) {
        assert(get_allocated(b) == FREE);

        uint32_t b_block_size = get_block_size(b);

        if (b_block_size >= free_block_size) {
            return b;
        } else {
            b = explicit_list->get_next_node(b);
        }
    }

    // 当前空间不存在合适的空闲块，返回NIL
    return NIL;
}

/* ------------------------------------- */
/*  Implementation                       */
/* ------------------------------------- */

bool explicit_list_initialize_free_block() {
    explicit_list.reset(new EXPLICIT_FREE_LINKED_LIST(NULL_NODE, 0));

    uint64_t first_header = get_first_block();

    explicit_list->insert_node(first_header);

    // init small block list
    small_list_init();

    return true;
}

uint64_t explicit_list_search_free_block(uint32_t payload_size, uint32_t &alloc_block_size) {
    // search 8-byte block list
    if (payload_size <= 4) {
        // a small block
        alloc_block_size = 8;

        if (small_list->count()) {
            // 8-byte list is not empty
            return small_list->head();
        }
    } else {
        alloc_block_size = round_up(payload_size, 8) + 4 + 4;
        assert(alloc_block_size >= MIN_EXPLICIT_FREE_LIST_BLOCKSIZE);
    }

    // search explicit free list
    return explicit_list_search(alloc_block_size);
}

bool explicit_list_insert_free_block(uint64_t free_header) {
    assert(free_header % 8 == 4);
    assert(get_first_block() <= free_header && free_header <= get_last_block());
    assert(get_allocated(free_header) == FREE);

    uint32_t block_size = get_block_size(free_header);
    assert(block_size % 8 == 0);
    assert(block_size >= 8);

    switch (block_size) {
        case 8:
            small_list_insert(free_header);
            break;
        default:
            assert(block_size >= MIN_EXPLICIT_FREE_LIST_BLOCKSIZE);
            explicit_list->insert_node(free_header);
            break;
    }

    return true;
}

bool explicit_list_delete_free_block(uint64_t free_header) {
    assert(free_header % 8 == 4);
    assert(get_first_block() <= free_header && free_header <= get_last_block());
    assert(get_allocated(free_header) == FREE);

    uint32_t block_size = get_block_size(free_header);
    assert(block_size % 8 == 0);
    assert(block_size >= 8);

    switch (block_size) {
        case 8:
            small_list_delete(free_header);
            break;
        default:
            assert(block_size >= MIN_EXPLICIT_FREE_LIST_BLOCKSIZE);
            explicit_list->delete_node(free_header);
            break;
    }

    return true;
}

void explicit_list_check_free_block() {
    small_list_check_free_blocks();
    check_size_list_correctness(explicit_list, MIN_EXPLICIT_FREE_LIST_BLOCKSIZE, 0xFFFFFFFF);
}

/* ------------------------------------- */
/*  For Debugging                        */
/* ------------------------------------- */
static void explicit_list_print() {
    uint64_t p = explicit_list->get_next();
    printf("explicit free list <{%lu},{%lu}>:\n", p, explicit_list->count());
    for (int i = 0; i < explicit_list->count(); ++i) {
        printf("<%lu:%u/%u> ", p, get_block_size(p), get_allocated(p));
        p = explicit_list->get_next();
    }
    printf("\n");
}