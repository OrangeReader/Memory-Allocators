#include <cstdio>

#include "allocator.h"
#include "linked-list.h"

//extern int heap_init();
//extern uint64_t mem_alloc(uint32_t size);
//extern void mem_free(uint64_t payload_vaddr);

/* ------------------------------------- */
/*  Unit tests                           */
/* ------------------------------------- */
static void test_roundup() {
    printf("Testing round up ...\n");

    for (int i = 0; i < 100; ++i) {
        for (int j = 1; j <= 8; ++j) {
            uint32_t x = i * 8 + j;
            assert(round_up(x, 8) == (i + 1) * 8);
        }
    }

    /* 控制字符的通用格式如下:
     * `Esc[{attr1};...;{attrn}m`
     * 其中:
     * Esc 是转义字符, 其值为"\033";
     * [ 是左中括号;
     * {attr1};...{attrn} 是若干属性, 通常是由一个有特定意义的数字代替,
     * 每个属性之间用分号分隔; m 就是字面常量字符m;
     *
     * \033[32m  设置字体颜色为绿色
     * \033[1m   设置高亮/加粗
     * \033[0m   关闭所有属性
     */
    printf("\033[32;1m\tPass\033[0m\n");
}

/*  hex table
    0       0000    *
    1       0001    *
    2       0010
    3       0011
    4       0100
    5       0101
    6       0110
    7       0111
    8       1000    *
    9       1001    *
    a   10  1010
    b   11  1011
    c   12  1100
    d   13  1101
    e   14  1110
    f   15  1111
 */
static void test_get_block_size_allocated() {
    printf("Testing getting block size from header ...\n");

    for (int i = get_prologue(); i < get_epilogue(); i += 4) {
        *(uint32_t *) &heap[i] = 0x1234abc0;
        assert(get_block_size(i) == 0x1234abc0);
        assert(get_allocated(i) == 0);

        *(uint32_t *) &heap[i] = 0x1234abc1;
        assert(get_block_size(i) == 0x1234abc0);
        assert(get_allocated(i) == 1);

        *(uint32_t *) &heap[i] = 0x1234abc8;
        assert(get_block_size(i) == 0x1234abc8);
        assert(get_allocated(i) == 0);

        *(uint32_t *) &heap[i] = 0x1234abc9;
        assert(get_block_size(i) == 0x1234abc8);
        assert(get_allocated(i) == 1);
    }

    printf("\033[32;1m\tPass\033[0m\n");
}

static void test_set_block_size_allocated() {
    printf("Testing setting block size to header ...\n");

    for (int i = get_prologue(); i < get_epilogue(); i += 4) {
        set_block_size(i, 0x1234abc0);
        set_allocated(i, 0);
        assert(get_block_size(i) == 0x1234abc0);
        assert(get_allocated(i) == 0);

        set_block_size(i, 0x1234abc0);
        set_allocated(i, 1);
        assert(get_block_size(i) == 0x1234abc0);
        assert(get_allocated(i) == 1);

        set_block_size(i, 0x1234abc8);
        set_allocated(i, 0);
        assert(get_block_size(i) == 0x1234abc8);
        assert(get_allocated(i) == 0);

        set_block_size(i, 0x1234abc8);
        set_allocated(i, 1);
        assert(get_block_size(i) == 0x1234abc8);
        assert(get_allocated(i) == 1);
    }

    // set the block size of last block
    for (int i = 2; i < 100; ++i) {
        uint32_t block_size = i * 8;
        uint64_t addr = get_epilogue() - block_size;

        set_block_size(addr, block_size);
        assert(get_block_size(addr) == block_size);
        assert(is_last_block(addr) == 1);
    }

    printf("\033[32;1m\tPass\033[0m\n");
}

static void test_get_header_payload_addr() {
    printf("Testing getting header or payload virtual addresses ...\n");

    uint64_t header_addr, payload_addr;
    for (int i = get_payload(get_first_block()); i < get_epilogue(); i += 8) {
        payload_addr = i;
        header_addr = payload_addr - 4;

        assert(get_payload(header_addr) == payload_addr);
        assert(get_payload(payload_addr) == payload_addr);

        assert(get_header(header_addr) == header_addr);
        assert(get_header(payload_addr) == header_addr);
    }

    printf("\033[32;1m\tPass\033[0m\n");
}

static void test_get_next_prev() {
    printf("Testing linked list traversal ...\n");

    srand(123456);

    // let me setup the heap first
    heap_init();

    uint64_t h = get_first_block();
    uint64_t f = 0;

    uint32_t collection_block_size[1000];
    uint32_t collection_allocated[1000];
    uint32_t collection_headeraddr[1000];
    int counter = 0;

    uint32_t allocated = 1;
    uint64_t epilogue = get_epilogue();
    while (h < epilogue) {
        uint32_t block_size = 8 * (1 + rand() % 16);
        // we don't split the last space(<= 64)
        if (epilogue - h <= 64) {
            block_size = epilogue - h;
        }

        if (allocated == 1 && (rand() % 3) >= 1) {
            // with previous allocated, 2/3 possibilities to be free
            allocated = 0;
        } else {
            allocated = 1;
        }

        collection_block_size[counter] = block_size;
        collection_allocated[counter] = allocated;
        collection_headeraddr[counter] = h;
        counter += 1;

        set_allocated(h, allocated);
        set_block_size(h, block_size);

        f = h + block_size - 4;
        set_allocated(f, allocated);
        set_block_size(f, block_size);

        h = h + block_size;
    }

    // check get_next
    h = get_first_block();
    int i = 0;
    while (h != 0 && h < get_epilogue()) {
//        printf("now header = %lld\n", h);
        assert(i <= counter);
        assert(h == collection_headeraddr[i]);
        // TODO: WHEN h = 1748, next P8 is not set correctly
        assert(get_block_size(h) == collection_block_size[i]);
        assert(get_allocated(h) == collection_allocated[i]);

        uint64_t temp = get_next_header(h);
        h = get_next_header(h);
        i += 1;
    }

    // check get_prev
    h = get_last_block();
    i = counter - 1;
    while (h != 0 && get_first_block() <= h) {
//        printf("now header = %lld\n", h);
        assert(0 <= i);
        assert(h == collection_headeraddr[i]);
        assert(get_block_size(h) == collection_block_size[i]);
        assert(get_allocated(h) == collection_allocated[i]);

        h = get_prev_header(h);
        i -= 1;
    }

    printf("\033[32;1m\tPass\033[0m\n");
}

static void test_malloc_free() {
#if defined(IMPLICIT_FREE_LIST)
    printf("Testing implicit list malloc & free ...\n");
#endif

#if defined(EXPLICIT_FREE_LIST)
    printf("Testing explicit list malloc & free ...\n");
#endif

    heap_init();

    srand(42);

    // collection for the pointers
    INT_LINKED_LIST *ptrs = new INT_LINKED_LIST(NULL_NODE, 0);

//    bool prev = false;
    for (int i = 0; i < 100000; ++i) {
        uint32_t size = rand() % 1024 + 1; // a non zero value

        if ((rand() & 0x1) == 0) {

//            if (size == 800 && prev) {
//                print_heap();
//            } else {
//                prev = false;
//            }
//
//            if (size == 878) {
//                prev = true;
//            }

            // malloc, return the payload address
            uint64_t p = mem_alloc(size);
//            printf("\tmalloc: payload = %lu, size = %u\n", p, size);

            if (p != 0) {
                uint64_t temp = (uint64_t)(new INT_LINKED_LIST_NODE(p));
                ptrs->insert_node(temp);
            }
        } else if (ptrs->count() != 0) {
            // free
            // randomly select one to free
            int random_index = rand() % ptrs->count();
            uint64_t t = ptrs->get_node_by_index(random_index);
            uint64_t t_value = ((int_linked_list_node_t *)t)->value;

//            printf("\tfree: payload = %lu\n", t_value);
            mem_free(t_value);

            ptrs->delete_node(t);
        }
    }

//    printf("next checking heap\n\n");

    // 对剩下的全部进行free
    int num_still_allocated = ptrs->count();
    for (int i = 0; i < num_still_allocated; ++i) {
        uint64_t t = ptrs->get_next();
        uint64_t t_value = ((int_linked_list_node_t *)t)->value;

        mem_free(t_value);
        ptrs->delete_node(t);
    }
    assert(ptrs->count() == 0);
    delete ptrs;

    // finally there should be only one free block
    assert(is_last_block(get_first_block()) == true);
    assert(get_allocated(get_first_block()) == FREE);

    printf("\033[32;1m\tPass\033[0m\n");
}

int main() {
    test_roundup();
    test_get_block_size_allocated();
    test_set_block_size_allocated();
    test_get_header_payload_addr();
    test_get_next_prev();

    test_malloc_free();

    return 0;
}