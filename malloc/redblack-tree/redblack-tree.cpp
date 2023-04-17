#include <cassert>
#include <memory>

#include "allocator.h"
#include "redblack-tree.h"

/* ------------------------------------- */
/*  Operations for Tree Block Structure  */
/* ------------------------------------- */
uint64_t FREE_RBT::get_root() const {
    return root_;
}

bool FREE_RBT::is_null_node(uint64_t header_vaddr) const {
    if (get_first_block() <= header_vaddr &&
        header_vaddr <= get_last_block() &&
        header_vaddr % 8 == 4) {
        return true;
    }

    return false;
}

bool FREE_RBT::set_root(uint64_t new_root) {
    root_ = new_root;
    return true;
}

// FREE_RBT中不需要构建和销毁节点
uint64_t FREE_RBT::construct_node() {
    return NULL_NODE;
}

// 将其转化为隐式链表
bool FREE_RBT::destruct_node(uint64_t header_vaddr) {
    if (header_vaddr == NIL) {
        return false;
    }

    // 可以不用管，兼容隐式链表格式
    return true;
}

bool FREE_RBT::is_nodes_equal(uint64_t first, uint64_t second) {
    return first == second;
}

// node is header_vaddr
uint64_t FREE_RBT::get_parent(uint64_t node) const {
    return get_field32_block_ptr(node, MIN_REDBLACK_TREE_BLOCKSIZE, 4);
}

bool FREE_RBT::set_parent(uint64_t node, uintptr_t parent) {
    return set_field32_block_ptr(node, parent, MIN_REDBLACK_TREE_BLOCKSIZE, 4);
}

uint64_t FREE_RBT::get_left_child(uint64_t node) const {
    return get_field32_block_ptr(node, MIN_REDBLACK_TREE_BLOCKSIZE, 8);
}

bool FREE_RBT::set_left_child(uint64_t node, uint64_t left_child) {
    return set_field32_block_ptr(node, left_child, MIN_REDBLACK_TREE_BLOCKSIZE, 8);
}

uint64_t FREE_RBT::get_right_child(uint64_t node) const {
    return get_field32_block_ptr(node, MIN_REDBLACK_TREE_BLOCKSIZE, 12);
}

bool FREE_RBT::set_right_child(uint64_t node, uint64_t right_child) {
    return set_field32_block_ptr(node, right_child, MIN_REDBLACK_TREE_BLOCKSIZE, 12);
}

rbt_color_t FREE_RBT::get_color(uint64_t node) const {
    if (node == NIL) {
        // default BLACK
        return COLOR_BLACK;
    }

    assert(get_prologue() <= node && node << get_epilogue());
    assert(node % 8 == 4);  // 传入的是block起始地址，而非payload地址
    assert(get_block_size(node) >= MIN_REDBLACK_TREE_BLOCKSIZE);

    uint64_t footer_vaddr = get_footer(node);
    uint32_t footer_value = *(reinterpret_cast<uint32_t *>(&heap[footer_vaddr]));

    return static_cast<rbt_color_t>((footer_value >> 1) & 0x1);
}

bool FREE_RBT::set_color(uint64_t node, rbt_color_t color) {
    if (node == NIL) {
        return false;
    }

    assert(color == COLOR_BLACK || color == COLOR_RED);
    assert(get_prologue() <= node && node <= get_epilogue());
    assert(node % 8 == 4);
    assert(get_block_size(node) >= MIN_REDBLACK_TREE_BLOCKSIZE);

    uint64_t footer_vaddr = get_footer(node);
    *(reinterpret_cast<uint32_t *>(&heap[footer_vaddr])) &= 0xFFFFFFFD; // 0xD = 1101 将color位清空
    *(reinterpret_cast<uint32_t *>(&heap[footer_vaddr])) |= ((color & 0x1) << 1);   // set color

    return true;
}

// key is block size
uint64_t FREE_RBT::get_key(uint64_t node) const {
    uint32_t block_size = get_block_size(node);
    return block_size;
}

bool FREE_RBT::set_key(uint64_t node, uint64_t key) {
    assert((key & 0xFFFFFFFF00000000) == 0);

    set_block_size(node, key);
    return true;
}

// FREE_RBT中无需设置value
bool FREE_RBT::set_value(uint64_t node, uint64_t value) {
    return false;
}

uint64_t FREE_RBT::get_value(uint64_t node) const {
    return NIL;
}

// The red-black tree
static std::unique_ptr<FREE_RBT> rbt;

/* ------------------------------------- */
/*  Implementation                       */
/* ------------------------------------- */
