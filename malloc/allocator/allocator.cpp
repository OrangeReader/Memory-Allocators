#include <cassert>
#include <cstdio>

#include "allocator.h"

// 初始值不应该在此处设置(heap init中),下面设置只是为了通过单元测试
uint64_t heap_start_vaddr = 0;      // for pass uint-test
uint64_t heap_end_vaddr = 4096;     // for pass uint-test
uint8_t heap[HEAP_MAX_SIZE];

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

/* ------------------------------------- */
/*  Free Block Management Implementation */
/* ------------------------------------- */

#ifdef IMPLICIT_FREE_LIST
bool implicit_list_initialize_free_block();
uint64_t implicit_list_search_free_block(uint32_t payload_size, uint32_t &alloc_block_size);
bool implicit_list_insert_free_block(uint64_t free_header);
bool implicit_list_delete_free_block(uint64_t free_header);
void implicit_list_check_free_block();
#endif

#ifdef EXPLICIT_FREE_LIST
bool explicit_list_initialize_free_block();
uint64_t explicit_list_search_free_block(uint32_t payload_size, uint32_t &alloc_block_size);
bool explicit_list_insert_free_block(uint64_t free_header);
bool explicit_list_delete_free_block(uint64_t free_header);
void explicit_list_check_free_block();
#endif

#ifdef REDBLACK_TREE
bool redblack_tree_initialize_free_block();
uint64_t redblack_tree_search_free_block(uint32_t payload_size, uint32_t &alloc_block_size);
bool redblack_tree_insert_free_block(uint64_t free_header);
bool redblack_tree_delete_free_block(uint64_t free_header);
void redblack_tree_check_free_block();
#endif

static bool initialize_free_block() {
#ifdef IMPLICIT_FREE_LIST
    return implicit_list_initialize_free_block();
#endif

#ifdef EXPLICIT_FREE_LIST
    return explicit_list_initialize_free_block();
#endif

#ifdef REDBLACK_TREE
    return redblack_tree_initialize_free_block();
#endif
}

static uint64_t search_free_block(uint32_t payload_size, uint32_t &alloc_block_size) {
#ifdef IMPLICIT_FREE_LIST
    return implicit_list_search_free_block(payload_size, alloc_block_size);
#endif

#ifdef EXPLICIT_FREE_LIST
    return explicit_list_search_free_block(payload_size, alloc_block_size);
#endif

#ifdef REDBLACK_TREE
    return redblack_tree_search_free_block(payload_size, alloc_block_size);
#endif
}

static bool insert_free_block(uint64_t free_header) {
#ifdef IMPLICIT_FREE_LIST
    return implicit_list_insert_free_block(free_header);
#endif

#ifdef EXPLICIT_FREE_LIST
    return explicit_list_insert_free_block(free_header);
#endif

#ifdef REDBLACK_TREE
    return redblack_tree_insert_free_block(free_header);
#endif
}

static int delete_free_block(uint64_t free_header) {
#ifdef IMPLICIT_FREE_LIST
    return implicit_list_delete_free_block(free_header);
#endif

#ifdef EXPLICIT_FREE_LIST
    return explicit_list_delete_free_block(free_header);
#endif

#ifdef REDBLACK_TREE
    return redblack_tree_delete_free_block(free_header);
#endif
}

static void check_free_block() {
#ifdef IMPLICIT_FREE_LIST
    implicit_list_check_free_block();
#endif

#ifdef EXPLICIT_FREE_LIST
    explicit_list_check_free_block();
#endif

#ifdef REDBLACK_TREE
    redblack_tree_check_free_block();
#endif
}

/* ------------------------------------- */
/*  Malloc and Free                      */
/* ------------------------------------- */
void check_heap_correctness();

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

    // 当合并存在8-Byte时，由于合并后大小不再是8Byte，因此在设置header的size时会清除后面的P8信息
    // 在get_footer中又会去检查8-Bytes完整性，导致矛盾
    // 因此此处使用low
    uint64_t footer = get_footer(low);
    set_block_size(footer, block_size);
    set_allocated(footer, FREE);

    return low;
}

// 首次适配内存分配算法：找到第一块合适的block用于分配
uint64_t try_alloc_with_splitting(uint64_t block_vaddr, uint32_t request_block_size) {
    if (request_block_size < 8) {
        return NIL;
    }

    uint64_t b = block_vaddr;
    uint32_t b_block_size = get_block_size(b);
    uint32_t b_allocated = get_allocated(b);

    if (b_allocated == FREE && b_block_size >= request_block_size) {
        // allocated this block
        delete_free_block(b);
        uint64_t right_footer = get_footer(b);

        set_allocated(b, ALLOCATED);
        set_block_size(b, request_block_size);

        uint64_t b_footer = b + request_block_size - 4;
        set_allocated(b_footer, ALLOCATED);
        set_block_size(b_footer, request_block_size);

        // 当剩余部分小于8 Byte，任何结构都无法满足，最低要求至少有8 Byte
        // 不存在这种情况，因为分配的空间要求8 Byte对齐，因此要分配后剩余空间小于 8 Byte，则会round up到全部大小
        uint32_t right_size = b_block_size - request_block_size;
        if (right_size >= 8) {
            // split this block `b`
            // b_block_size - request_block_size >= 8
            uint64_t right_header = get_next_header(b);

            set_allocated(right_header, FREE);
            set_block_size(right_header, right_size);

            set_allocated(right_footer, FREE);
            set_block_size(right_footer, right_size);

            assert(get_footer(right_header) == right_footer);

            insert_free_block(right_header);
        }
        return get_payload(b);
    }

    // 当前空间无法满足分配或是已分配
    return NIL;
}

uint64_t try_extend_heap_to_alloc(uint32_t size) {
    // get the size to be added
    uint64_t old_last = get_last_block();

    uint32_t last_allocated = get_allocated(old_last);
    uint32_t last_block_size = get_block_size(old_last);

    uint32_t to_request_from_OS = size;
    if (last_allocated == FREE) {
        // last block can help the request
        to_request_from_OS -= last_block_size;

        delete_free_block(old_last);
    }

    uint64_t old_epilogue = get_epilogue();

    uint32_t os_allocated_size = extend_heap(to_request_from_OS);
    if (os_allocated_size) {
        // 成功进行heap拓展，将新申请的空间 + [原始末尾空闲块] -> 合并 + 管理起来
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

            insert_free_block(new_last);

            block_header = new_last;
        } else {
            // merging with last_block is needed
            set_allocated(old_last, FREE);
            set_block_size(old_last, last_block_size + os_allocated_size);

            uint64_t last_footer = get_footer(old_last);
            set_allocated(last_footer, FREE);
            set_block_size(last_footer, last_block_size + os_allocated_size);

            // block size is different now
            // consider the balanced tree index on block size, it must be reinserted
            insert_free_block(old_last);

            block_header = old_last;
        }

        // try to allocate
        uint64_t payload_vaddr = try_alloc_with_splitting(block_header, size);
        if (payload_vaddr != NIL)
        {
#ifdef DEBUG_MALLOC
            check_heap_correctness();
#endif
            return payload_vaddr;
        } else {
            assert(false);
        }
    }

    // else, no page can be allocated
    // 将原来末尾的最后一块空闲块插入回去
    if (last_allocated == FREE) {
        insert_free_block(old_last);
    }

#ifdef DEBUG_MALLOC
    check_heap_correctness();
    printf("OS cannot allocate physical page for heap!\n");
#endif

    return NIL;
}

// interface
bool heap_init() {
    // reset allocated to 0
    for (int i = 0; i < HEAP_MAX_SIZE; i += 8) {
        *(uint64_t *) &heap[i] = 0;
    }

    // heap_start_vaddr is the starting address of the first block
    // the payload of the first block is 8B aligned ([8])
    // so the header address of the first block is [8] - 4 = [4]
    heap_start_vaddr = 0;
    heap_end_vaddr = 4096;

    // set the prologue block
    uint64_t prologue_header = get_prologue();
    set_block_size(prologue_header, 8);
    set_allocated(prologue_header, ALLOCATED);

    uint64_t prologue_footer = prologue_header + 4;
    set_block_size(prologue_footer, 8);
    set_allocated(prologue_footer, ALLOCATED);

    // set the epilogue block
    // it's a header only
    uint64_t epilogue = get_epilogue();
    set_block_size(epilogue, 0);
    set_allocated(epilogue, ALLOCATED);

    // set the block size & allocated of the only regular block
    uint64_t first_header = get_first_block();
    set_block_size(first_header, 4096 - 4 - 8 - 4);
    set_allocated(first_header, FREE);

    uint64_t first_footer = get_footer(first_header);
    set_block_size(first_footer, 4096 - 4 - 8 - 4);
    set_allocated(first_footer, FREE);

    initialize_free_block();

    return true;
}

uint64_t mem_alloc(uint32_t size) {
    assert(0 < size && size < HEAP_MAX_SIZE - 4 - 8 - 4);

    uint32_t alloc_block_size = 0;
    // 在当前heap中寻找合适的free_block，如果不存在则返回NIL
    uint64_t payload_header = search_free_block(size, alloc_block_size);
    uint64_t payload_vaddr = NIL;

    if (payload_header != NIL) {
        // 找到了合适的空闲块
        payload_vaddr = try_alloc_with_splitting(payload_header, alloc_block_size);
        assert(payload_vaddr != NIL);
    } else {
        // 没有合适的空闲块，进行系统调用 system_brk()
        payload_vaddr = try_extend_heap_to_alloc(alloc_block_size);
        // 可能 OS 没有多余的内存的了，会返回NIL
    }


#ifdef DEBUG_MALLOC
    check_heap_correctness();
    check_free_block();
#endif

    return payload_vaddr;
}

void mem_free(uint64_t payload_vaddr) {
    if (payload_vaddr == NIL) {
        return;
    }

    assert(get_first_block() < payload_vaddr && payload_vaddr < get_epilogue());
    assert((payload_vaddr & 0x7) == 0x0);

    // request can be first or last block
    uint64_t req = get_header(payload_vaddr);
    uint64_t req_footer = get_footer(req);  // for last block, it's 0

    uint32_t req_allocated = get_allocated(req);
    uint32_t req_block_size = get_block_size(req);

    // otherwise it's free twice
    assert(req_allocated == ALLOCATED);

    // block starting address of next & prev blocks
    uint64_t next = get_next_header(req);
    uint64_t prev = get_prev_header(req);

    uint32_t next_allocated = get_allocated(next);
    uint32_t prev_allocated = get_allocated(prev);

    if (next_allocated == ALLOCATED && prev_allocated == ALLOCATED) {
        // case 1: *A(A->F)A*
        // ==> *AFA*
        set_allocated(req, FREE);
        set_allocated(req_footer, FREE);

        // 更新空闲块信息
        insert_free_block(req);
#ifdef DEBUG_MALLOC
        check_heap_correctness();
        check_free_block();
#endif
    } else if (next_allocated == FREE && prev_allocated == ALLOCATED) {
        // case 2: *A(A->F)FA
        // ==> *AFFA ==> *A[FF]A merge current and next
        delete_free_block(next);

        uint64_t one_free = merge_blocks_as_free(req, next);

        insert_free_block(one_free);
#ifdef DEBUG_MALLOC
        check_heap_correctness();
        check_free_block();
#endif
    } else if (next_allocated == ALLOCATED && prev_allocated == FREE) {
        // case 3: AF(A->F)A*
        // ==> AFFA* ==> A[FF]A* merge current and prev
        delete_free_block(prev);

        uint64_t one_free = merge_blocks_as_free(prev, req);

        insert_free_block(one_free);
#ifdef DEBUG_MALLOC
        check_heap_correctness();
        check_free_block();
#endif
    } else if (next_allocated == FREE && prev_allocated == FREE) {
        // case 4: AF(A->F)FA
        // ==> AFFFA ==> A[FFF]A merge current and prev and next
        delete_free_block(prev);
        delete_free_block(next);

        uint64_t one_free = merge_blocks_as_free(merge_blocks_as_free(prev, req), next);

        insert_free_block(one_free);
#ifdef DEBUG_MALLOC
        check_heap_correctness();
        check_free_block();
#endif
    } else {
#ifdef DEBUG_MALLOC
        printf("exception for free\n");
        exit(0);
#endif
    }
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

        uint32_t block_size = get_block_size(p);
        if (block_size != 8) {
            assert(get_block_size(p) == get_block_size(f));
            assert(get_allocated(p) == get_allocated(f));
        }

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

static void block_info_print(uint64_t h) {
    uint32_t a = get_allocated(h);
    uint32_t s = get_block_size(h);
    uint64_t f = get_footer(h);

    uint32_t hv = *reinterpret_cast<uint32_t *>(&heap[h]);
    uint32_t fv = *reinterpret_cast<uint32_t *>(&heap[f]);

    uint32_t p8 = (hv >> 1) & 0x1;
    uint32_t b8 = (hv >> 2) & 0x1;
    uint32_t rb = (fv >> 1) & 0x1;

    printf("H:%lu,\tF:%lu,\tS:%u,\t(A:%u,RB:%u,B8:%u,P8:%u)\n", h, f, s, a, rb, b8, p8);
}

void print_heap() {
    printf("============\nheap blocks:\n");
    uint64_t h = get_first_block();
    int i = 0;
    while (i < (HEAP_MAX_SIZE / 8) && h != NIL && h < get_epilogue()) {
        block_info_print(h);
        h = get_next_header(h);

        i += 1;
        if (i % 5 == 0) {
            printf("\b\n");
        }
    }
    printf("\b\b\n============\n");
}