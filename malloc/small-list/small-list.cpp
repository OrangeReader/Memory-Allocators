#include <memory>

#include "allocator.h"
#include "small-list.h"


/* ------------------------------------- */
/*  Operations for List Block Structure  */
/* ------------------------------------- */
uint64_t SMALL_FREE_LINKED_LIST::get_head() const {
    return head_;
}

bool SMALL_FREE_LINKED_LIST::set_head(uint64_t new_head) {
    head_ = new_head;
    return true;
}

uint64_t SMALL_FREE_LINKED_LIST::get_count() const {
    return count_;
}

bool SMALL_FREE_LINKED_LIST::set_count(uint64_t new_count) {
    count_ = new_count;
    return true;
}

bool SMALL_FREE_LINKED_LIST::destruct_node(uint64_t node) {
    return true;
}

bool SMALL_FREE_LINKED_LIST::is_nodes_equal(uint64_t first, uint64_t second) {
    return first == second;
}

uint64_t SMALL_FREE_LINKED_LIST::get_node_prev(uint64_t node) {
    assert(node % 8 == 4);
    assert(get_allocated(node) == FREE);

    uint32_t value = *reinterpret_cast<uint32_t *>(&heap[node]);
    // 由于8-Byte中保存size地方现用来保存prev和next
    // ⭐ size是8字节对齐，而prev和next是4字节对齐，存入少了4，因此我们需要给它加回来
    return 4 + (value & 0xFFFFFFF8);
}

bool SMALL_FREE_LINKED_LIST::set_node_prev(uint64_t node, uint64_t prev) {
    // we set by header only
    assert(node % 8 == 4);
    assert(get_allocated(node) == FREE);
    assert(prev % 8 == 4);

    // ⭐ 注意：这里将header_addr设置成了8字节对齐，会抹去最后一个1，即x100 -> x000
    // 后续获取prev和next的时候需要设置回来
    *reinterpret_cast<uint32_t *>(&heap[node]) &= 0x00000007;   // reset prev pointer
    *reinterpret_cast<uint32_t *>(&heap[node]) |= (prev & 0xFFFFFFF8);

    return true;
}

uint64_t SMALL_FREE_LINKED_LIST::get_node_next(uint64_t node) {
    assert(node % 8 == 4);
    assert(get_allocated(node) == FREE);

    uint32_t value = *reinterpret_cast<uint32_t *>(&heap[node + 4]);
    // 由于8-Byte中保存size地方现用来保存prev和next
    // ⭐ size是8字节对齐，而prev和next是4字节对齐，存入少了4，因此我们需要给它加回来
    return 4 + (value & 0xFFFFFFF8);
}

bool SMALL_FREE_LINKED_LIST::set_node_next(uint64_t node, uint64_t next) {
    assert(node % 8 == 4);
    assert(get_allocated(node) == FREE);
    assert(next % 8 == 4);

    // 设置next
    node += 4;
    // ⭐ 注意：这里将header_addr设置成了8字节对齐，会抹去最后一个1，即x100 -> x000
    // 后续获取prev和next的时候需要设置回来
    *reinterpret_cast<uint32_t *>(&heap[node]) &= 0x00000007;   // reset next pointer
    *reinterpret_cast<uint32_t *>(&heap[node]) |= (next & 0xFFFFFFF8);

    return true;
}

/* ------------------------------------- */
/*  Operations for Linked List           */
/* ------------------------------------- */

// unique_ptr 并不支持 多态的向上转换(子类->父类), 因此改为shared_ptr
std::shared_ptr<SMALL_FREE_LINKED_LIST> small_list;

void small_list_init() {
    small_list.reset(new SMALL_FREE_LINKED_LIST(NULL_NODE, 0));
}

void small_list_insert(uint64_t free_header) {
    assert(get_first_block() <= free_header && free_header <= get_last_block());
    assert(free_header % 8 == 4);
    assert(get_block_size(free_header) == 8);
    assert(get_allocated(free_header) == FREE);

    small_list->insert_node(free_header);
}

void small_list_delete(uint64_t free_header) {
    assert(get_first_block() <= free_header && free_header <= get_last_block());
    assert(free_header % 8 == 4);
    assert(get_block_size(free_header) == 8);

    small_list->delete_node(free_header);
}

// 参考原始explicit-list中 `check_explicit_list_correctness`
// check 链表管理的空闲块是否符合要求 + 链表的检查
// 1. small-list的大小均为8-Byte
// 2. explicit-list
// 2.1 explicit-list大小应处于 [16, 0xFFFFFFFF]  in explicit list
// 2.2 explicit-list大小应处于 [16, 0x32]        in red black tree
void check_size_list_correctness(const std::shared_ptr<LINKED_LIST> &list, uint32_t min_size, uint32_t max_size) {
    uint32_t counter = 0;
    uint64_t b = get_first_block();
    bool head_exists = false;

    while (b <= get_last_block()) {
        uint32_t b_block_size = get_block_size(b);

        if (get_allocated(b) == FREE && min_size <= b_block_size && b_block_size <= max_size) {
            uint64_t prev = list->get_prev_node(b);
            uint64_t next = list->get_next_node(b);
            uint64_t prev_next = list->get_next_node(prev);
            uint64_t next_prev = list->get_prev_node(next);

            assert(get_allocated(prev) == FREE);
            assert(get_allocated(next) == FREE);
            assert(prev_next == b);
            assert(next_prev == b);

            if (b == list->head()) {
                head_exists = true;
            }

            ++counter;
        }

        b = get_next_header(b);
    }

    assert(list->count() == 0 || head_exists);
    assert(list->count() == counter);

    uint64_t p = list->head();
    uint64_t n = list->head();
    for (int i = 0; i < list->count(); ++i) {
        uint32_t p_size = get_block_size(p);
        uint32_t n_size = get_block_size(n);

        assert(get_allocated(p) == FREE);
        assert(min_size <= p_size && p_size <= max_size);

        assert(get_allocated(n) == FREE);
        assert(min_size <= n_size && n_size <= max_size);

        p = list->get_prev_node(p);
        n = list->get_next_node(n);
    }
    assert(p == list->head());
    assert(n == list->head());
}

void small_list_check_free_blocks() {
    check_size_list_correctness(small_list, 8, 8);
}