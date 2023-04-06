#include "allocator.h"
#include "explicit-list.h"
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
#ifdef EXPLICIT_FREE_LIST

int heap_init() {
    return internal_heap_init();
}

uint64_t mem_alloc(uint32_t size) {
    return internal_malloc(size);
}

void mem_free(uint64_t payload_vaddr) {
    internal_free(payload_vaddr);
}

#endif

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

const uint32_t MIN_EXPLICIT_FREE_LIST_BLOCKSIZE = 16;

/* ------------------------------------- */
/*  Operations for List Block Structure  */
/* ------------------------------------- */
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
static EXPLICIT_FREE_LINKED_LIST *explicit_list = nullptr;

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

#if defined(DEBUG_MALLOC) && defined(EXPLICIT_FREE_LIST)
void on_sigabrt(int signum)
{
    // like a try-catch for the asserts
    printf("%s\n", debug_message);
    print_heap();
    explicit_list_print();
    exit(0);
}
#endif

static void check_explicit_list_correctness() {
    uint32_t free_counter = 0;
    uint64_t p = get_first_block();
    while (p != NIL && p <= get_last_block()) {
        if (get_allocated(p) == FREE) {
            free_counter += 1;

            assert(get_block_size(p) >= MIN_EXPLICIT_FREE_LIST_BLOCKSIZE);
            assert(get_allocated(explicit_list->get_prev_node(p)) == FREE);
            assert(get_allocated(explicit_list->get_next_node(p)) == FREE);
        }

        p = get_next_header(p);
    }
    assert(free_counter == explicit_list->count());

    uint64_t explicit_list_head = explicit_list->get_node_by_index(0);
    p = explicit_list_head;
    uint64_t n = explicit_list_head;
    for (int i = 0; i < explicit_list->count(); ++i) {
        assert(get_allocated(p) == FREE);
        assert(get_block_size(p) >= MIN_EXPLICIT_FREE_LIST_BLOCKSIZE);

        assert(get_allocated(n) == FREE);
        assert(get_block_size(n) >= MIN_EXPLICIT_FREE_LIST_BLOCKSIZE);

        p = explicit_list->get_prev_node(p);
        n = explicit_list->get_next_node(n);
    }
    assert(p == explicit_list->get_node_by_index(0));
    assert(n == explicit_list->get_node_by_index(0));
}


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


    explicit_list = new EXPLICIT_FREE_LINKED_LIST(NULL_NODE, 0);
    // 插入时由于是第一个节点，因此会将其prev 和 next 设置为自己
    explicit_list->insert_node(first_header);

#ifdef DEBUG_MALLOC
    // SIGABRT: 程序的异常终止
    // 此处是 assert若不满足条件，那么会自动调用abort()结束程序，此时会发出SIGABRT信号
    // 信号注册，当当前进程接收到SIGABRT信号后调用on_sigabrt函数
    signal(SIGABRT, &on_sigabrt);
#endif
    return 1;
}

// size - requested payload size
// return - the virtual address of payload
static uint64_t internal_malloc(uint32_t size) {
    assert(0 < size && size < HEAP_MAX_SIZE - 4 - 8 - 4);

    uint64_t payload_vaddr = NIL;
    uint32_t request_block_size = round_up(size, 8) + 4 + 4;

    uint64_t b = explicit_list->get_next();

    // not multi-thread safe
    uint64_t counter_copy = explicit_list->count();
    // 效率上的提升: T(explicit) <= 1/2 * T(implicit) [一个free至少有1个allocate]
    for (int i = 0; i < counter_copy; ++i) {
        uint32_t b_old_block_size = get_block_size(b);
        // 只是会将块进行分配 + 分割 -> 可用于隐式空闲链表，并没有管理起来空闲块，因此后面需要我们自己将空闲块使用显示空闲链表管理起来
        payload_vaddr = try_alloc_with_splitting(b, request_block_size, MIN_EXPLICIT_FREE_LIST_BLOCKSIZE);

        if (payload_vaddr != NIL) {
            uint32_t b_new_block_size = get_block_size(b);
            assert(b_new_block_size <= b_new_block_size);
            explicit_list->delete_node(b);

            // 如果等于说明这个free block 全部被分配给了所申请的
            if (b_old_block_size > b_new_block_size) {
                // b has been splitted
                uint64_t a = get_next_header(b);
                assert(get_allocated(a) == FREE);
                assert(get_block_size(a) == b_old_block_size - b_new_block_size);
                explicit_list->insert_node(a);
            }

#ifdef DEBUG_MALLOC
            check_heap_correctness();
            check_explicit_list_correctness();
#endif
            return payload_vaddr;
        } else {
            // go to next block
            // 首次适配
            b = explicit_list->get_next_node(b);

            // 下次适配(head指针通过get_next是不断改变的, 接着上一次分配查找的位置继续查找)
            // b = explicit_list->get_next();

            // 还有最差适配 —— 每次寻找最大的块来分配给(暂未实现)
        }
    }

    // when no enough free block for current heap
    // request a new free physical & virtual page from OS
    // 1. 如果最后一块是空闲块，那么就先从显示空闲链表中删除
    uint64_t old_last = get_last_block();
    if (get_allocated(old_last) == FREE) {
        explicit_list->delete_node(old_last);
    }

    // 2. 分配内存： 一切都以设置好，只是更新显示空闲链表
    payload_vaddr = try_extend_heap_to_alloc(request_block_size, MIN_EXPLICIT_FREE_LIST_BLOCKSIZE);

    // 3. 如果分配后还剩余空间，即最后一块是空闲块，那么就将这个空闲块添加到显示空闲链表中
    uint64_t new_last = get_last_block();
    if (get_allocated(new_last) == FREE) {
        explicit_list->insert_node(new_last);
    }

#ifdef DEBUG_MALLOC
    check_heap_correctness();
    check_explicit_list_correctness();
#endif

    return payload_vaddr;
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

        // 更新显示空闲链表
        explicit_list->insert_node(req);
#ifdef DEBUG_MALLOC
        check_heap_correctness();
        check_explicit_list_correctness();
#endif
    } else if (next_allocated == FREE && prev_allocated == ALLOCATED) {
        // case 2: *A(A->F)FA
        // ==> *AFFA ==> *A[FF]A merge current and next
        explicit_list->delete_node(next);

        uint64_t one_free = merge_blocks_as_free(req, next);

        explicit_list->insert_node(one_free);
#ifdef DEBUG_MALLOC
        check_heap_correctness();
        check_explicit_list_correctness();
#endif
    } else if (next_allocated == ALLOCATED && prev_allocated == FREE) {
        // case 3: AF(A->F)A*
        // ==> AFFA* ==> A[FF]A* merge current and prev
        explicit_list->delete_node(prev);

        uint64_t one_free = merge_blocks_as_free(prev, req);

        explicit_list->insert_node(one_free);
#ifdef DEBUG_MALLOC
        check_heap_correctness();
        check_explicit_list_correctness();
#endif
    } else if (next_allocated == FREE && prev_allocated == FREE) {
        // case 4: AF(A->F)FA
        // ==> AFFFA ==> A[FFF]A merge current and prev and next
        explicit_list->delete_node(prev);
        explicit_list->delete_node(next);

        uint64_t one_free = merge_blocks_as_free(merge_blocks_as_free(prev, req), next);

        explicit_list->insert_node(one_free);
#ifdef DEBUG_MALLOC
        check_heap_correctness();
        check_explicit_list_correctness();
#endif
    } else {
#ifdef DEBUG_MALLOC
        printf("exception for free\n");
#endif
        exit(0);
    }
}