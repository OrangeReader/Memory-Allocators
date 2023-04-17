#ifndef MYMALLOC_REDBLACK_TREE_H
#define MYMALLOC_REDBLACK_TREE_H

#include "rbt.h"

const uint64_t MIN_REDBLACK_TREE_BLOCKSIZE = 24;

// ================================================ //
//      The implementation of the free block rbt    //
// ================================================ //
class FREE_RBT final : public RBT {
public:
    // 构造函数
    FREE_RBT(uint64_t root)
        : root_(root) {}

    // 将其空闲部分变为隐式空闲链表，对于RBT_BLOCK是不需要做任何处理的
    ~FREE_RBT() override = default;

    uint64_t get_root() const override;

protected:
    bool is_null_node(uint64_t header_vaddr) const override;

    bool set_root(uint64_t new_root) override;

    uint64_t construct_node() override;
    bool destruct_node(uint64_t node) override;

    bool is_nodes_equal(uint64_t first, uint64_t second) override;

    uint64_t get_parent(uint64_t node) const override;
    bool set_parent(uint64_t node, uintptr_t parent) override;

    uint64_t get_left_child(uint64_t node) const override;
    bool set_left_child(uint64_t node, uint64_t left_child) override;

    uint64_t get_right_child(uint64_t node) const override;
    bool set_right_child(uint64_t node, uint64_t right_child) override;

    rbt_color_t get_color(uint64_t node) const override;
    bool set_color(uint64_t node, rbt_color_t color) override;

    uint64_t get_key(uint64_t node) const override;
    bool set_key(uint64_t node, uint64_t key) override;

    uint64_t get_value(uint64_t node) const override;
    bool set_value(uint64_t node, uint64_t value) override;

private:
    uint64_t root_ = NIL;
};

#endif //MYMALLOC_REDBLACK_TREE_H
