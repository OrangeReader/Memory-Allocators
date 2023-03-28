#include "../../include/allocator.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <csignal>

static int internal_heap_init();

static uint64_t internal_malloc(uint32_t size);

static void internal_free(uint64_t payload_vaddr);

/* ------------------------------------- */
/*  Implementation of the Interfaces     */
/* ------------------------------------- */
#ifdef IMPLICIT_FREE_LIST

int heap_init() {
    return internal_heap_init();
}

uint64_t mem_alloc(uint32_t size) {
    return internal_malloc(size);
}

void mem_free(uint64_t payload_vaddr) {
    internal_free(payload_vaddr);
}

#ifdef DEBUG_MALLOC

void on_sigabrt(int signum) {
    // like a try-catch for the asserts
    printf("%s\n", debug_message);
    print_heap();
    exit(0);
}

#endif

#endif

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

static int internal_heap_init() {
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

#ifdef DEBUG_MALLOC
    // SIGABRT: 程序的异常终止
    // 此处是 assert若不满足条件，那么会自动调用abort()结束程序，此时会发出SIGABRT信号
    // 信号注册，当当前进程接收到SIGABRT信号后调用on_sigabrt函数
    signal(SIGABRT, &on_sigabrt);
#endif
    return 0;
}

// size - requested payload size
// return - the virtual address of payload
static uint64_t internal_malloc(uint32_t size) {
    assert(0 < size && size < HEAP_MAX_SIZE - 4 - 8 - 4);

    uint64_t payload_vaddr = NIL;
    uint32_t request_block_size = round_up(size, 8) + 4 + 4;

    uint64_t b = get_first_block();
    uint64_t epilogue = get_epilogue();

    // not multi-thread safe
    while (b != NIL && b < epilogue) {
        payload_vaddr = try_alloc_with_splitting(b, request_block_size, MIN_IMPLICIT_FREE_LIST_BLOCK_SIZE);

        if (payload_vaddr != NIL) {
#ifdef DEBUG_MALLOC
            check_heap_correctness();
#endif
            return payload_vaddr;
        } else {
            // go to next block
            b = get_next_header(b);
        }
    }

    // when no enough free block for current heap
    // request a new free physical & virtual page from OS
    return try_extend_heap_to_alloc(request_block_size, MIN_IMPLICIT_FREE_LIST_BLOCK_SIZE);
}

static void internal_free(uint64_t payload_vaddr) {
    if (payload_vaddr == NIL) {
        return;
    }

    assert(get_first_block() < payload_vaddr && payload_vaddr < get_epilogue());
    assert((payload_vaddr & 0x7) == 0x0);

    // request can be first or last block
    uint64_t req = get_header(payload_vaddr);
    uint64_t req_footer = get_footer(req);
    uint32_t req_allocated = get_allocated(req);

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
#ifdef DEBUG_MALLOC
        check_heap_correctness();
#endif
    } else if (next_allocated == FREE && prev_allocated == ALLOCATED) {
        // case 2: *A(A->F)FA
        // ==> *AFFA ==> *A[FF]A merge current and next
        merge_blocks_as_free(req, next);
#ifdef DEBUG_MALLOC
        check_heap_correctness();
#endif
    } else if (next_allocated == ALLOCATED && prev_allocated == FREE) {
        // case 3: AF(A->F)A*
        // ==> AFFA* ==> A[FF]A* merge current and prev
        merge_blocks_as_free(prev, req);
#ifdef DEBUG_MALLOC
        check_heap_correctness();
#endif
    } else if (next_allocated == FREE && prev_allocated == FREE) {
        // case 4: AF(A->F)FA
        // ==> AFFFA ==> A[FFF]A merge current and prev and next
        uint64_t merged = merge_blocks_as_free(prev, req);
        merge_blocks_as_free(merged, next);
#ifdef DEBUG_MALLOC
        check_heap_correctness();
#endif
    } else {
#ifdef DEBUG_MALLOC
        printf("exception for free\n");
#endif
        exit(0);
    }
}