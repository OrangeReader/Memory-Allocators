#include <cassert>
#include <memory>

#include "allocator.h"
#include "redblack-tree.h"
#include "small-list.h"
#include "explicit-list.h"

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
        return false;
    }

    return true;
}

bool FREE_RBT::set_root(uint64_t new_root) {
    root_ = new_root;
    return true;
}

// FREE_RBT中不需要构建和销毁节点
uint64_t FREE_RBT::construct_node() {
    return NULL_TREE_NODE;
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
std::shared_ptr<FREE_RBT> rbt;

// The returned node should have the key >= target key
uint64_t redblack_tree_search(uint32_t key) {
    if (rbt == nullptr) {
        return NULL_TREE_NODE;
    }

    if (rbt->get_root() == NULL_TREE_NODE) {
        return NULL_TREE_NODE;
    }

    uint64_t p = rbt->get_root();

    uint64_t successor = NULL_TREE_NODE;
    // positive infinite. should be large enough
    uint64_t successor_key = 0xFFFFFFFFFFFFFFFF;

    while (p != NULL_TREE_NODE) {
        uint64_t p_key = rbt->get_node_key(p);
        if (key == p_key) {
            // return the first found key
            // ⭐ which is the most left node of equals
            return p;
        } else if (key < p_key) {
            if (p_key <= successor_key) {
                // key < p_key <= successor_key
                // p is more close to target key than
                // the recorded successor
                successor = p;
                successor_key = p_key;
            }

            // to left child: if key == p_key, the p is the most left node of equals
            p = rbt->get_node_left(p);
        } else {
            // n_key > p_key
            p = rbt->get_node_right(p);
        }
    }

    // if no node key >= target key, return NULL_TREE_NODE
    // else return the first bigger than key
    return successor;
}

/* ------------------------------------- */
/*  Implementation                       */
/* ------------------------------------- */
bool redblack_tree_initialize_free_block() {
    uint64_t first_header = get_first_block();

    // init rbt for block >= 40
    rbt.reset(new FREE_RBT(NULL_TREE_NODE));
    rbt->insert_node(first_header);

    // init list for small block size in [16, 32]
    explicit_list_initialize();

    // init list for small block size = 8
    small_list_init();

    return true;
}


uint64_t redblack_tree_search_free_block(uint32_t payload_size, uint32_t &alloc_block_size) {
    // search 8-byte block list
    if (payload_size <= 4) {
        // a small block
        alloc_block_size = 8;

        if (small_list->count()) {
            // small list is not empty
            return small_list->head();
        }
    } else {
        alloc_block_size = round_up(payload_size, 8) + 4 + 4;
    }

    // search explicit free list
    if (MIN_EXPLICIT_FREE_LIST_BLOCKSIZE <= alloc_block_size && alloc_block_size < MIN_REDBLACK_TREE_BLOCKSIZE) {
        uint64_t b = explicit_list_search(alloc_block_size);

        // 可能 explicit list 为空 + 在explicit list 中找不到合适的空闲块
        if (b != NIL) {
            return b;
        }
    }

    // search rbt
    // 最佳适配算法: 找到第一块比req大的
    return redblack_tree_search(alloc_block_size);
}

bool redblack_tree_insert_free_block(uint64_t free_header) {
    assert(free_header % 8 == 4);
    assert(get_first_block() <= free_header && free_header <= get_last_block());
    assert(get_allocated(free_header) == FREE);

    uint32_t block_size = get_block_size(free_header);
    assert(block_size % 8 == 0);
    assert(block_size >= 8);

    if (block_size == 8) {
        small_list_insert(free_header);
    } else if (16 <= block_size && block_size <= 32) {
        explicit_list_insert(free_header);
    } else if (40 <= block_size) {
        rbt->insert_node(free_header);
    } else {
        return false;
    }

    return true;
}

bool redblack_tree_delete_free_block(uint64_t free_header) {
    assert(free_header % 8 == 4);
    assert(get_first_block() <= free_header && free_header <= get_last_block());
    assert(get_allocated(free_header) == FREE);

    uint32_t block_size = get_block_size(free_header);
    assert(block_size % 8 == 0);
    assert(block_size >= 8);

    if (block_size == 8) {
        small_list_delete(free_header);
    } else if (16 <= block_size && block_size <= 32) {
        explicit_list_delete(free_header);
    } else if (40 <= block_size) {
        rbt->delete_node(free_header);
    } else {
        return false;
    }

    return true;
}

void redblack_tree_check_free_block() {
    small_list_check_free_blocks();
    check_size_list_correctness(explicit_list, MIN_EXPLICIT_FREE_LIST_BLOCKSIZE, 32);
}