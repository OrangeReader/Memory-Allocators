#ifndef MYMALLOC_EXPLICIT_LIST_H
#define MYMALLOC_EXPLICIT_LIST_H

#include "linked-list.h"

// ================================================ //
//   The implementation of the explict linked list  //
// ================================================ //
// 我们采用数组模拟heap，数组的index就是virtual address，从而直接定位到相应的块
class EXPLICIT_FREE_LINKED_LIST final : public LINKED_LIST  {
public:
    // construct function
    EXPLICIT_FREE_LINKED_LIST(uint64_t head, uint64_t count)
        : head_(head), count_(count) {}

    // 由于heap上的空闲块，如果全部释放掉那么就变成了隐式空闲链表
    // 但是EXPLICIT FREE BLOCK 兼容 隐式空闲块
    ~EXPLICIT_FREE_LINKED_LIST() override = default;

protected:
    // EXPLICIT_FREE_LINKED_LIST中需要override的函数
    uint64_t get_head() const override;
    bool set_head(uint64_t new_head) override;

    uint64_t get_count() const override;
    bool set_count(uint64_t new_count) override;

    bool destruct_node(uint64_t node) override;

    bool is_nodes_equal(uint64_t first, uint64_t second) override;

    uint64_t get_node_prev(uint64_t header_vaddr) override;
    bool set_node_prev(uint64_t header_vaddr, uint64_t prev_vaddr) override;

    uint64_t get_node_next(uint64_t header_vaddr) override;
    bool set_node_next(uint64_t header_vaddr, uint64_t next_vaddr) override;

private:
    uint64_t head_ = NIL;
    uint64_t count_ = 0;
};

#endif //MYMALLOC_EXPLICIT_LIST_H
