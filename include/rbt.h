#ifndef MYMALLOC_RBT_H
#define MYMALLOC_RBT_H

#include <cstdint>

const uint64_t NULL_NODE = 0;

using rbt_color_t = enum {
    COLOR_RED,      // 默认为RED NODE
    COLOR_BLACK,
};

using child_t = enum {
    LEFT_CHILD = 0,
    RIGHT_CHILD = 1,
};

// 基类的公有函数调用私有函数，再其派生类中该公用函数可以正常使用，无需重新定义相应的私有函数
class RBT {
public:
    // ========== 拷贝控制 ========== //
    RBT() = default;

    // 拷贝构造、赋值
    // 移动构造、赋值

    // 继承体系中，我们只需要删除相应的成员即可
    virtual ~RBT() = default;

    void insert_node(uint64_t node);
    void delete_node(uint64_t node);

    bool tree_compare(uint64_t lhs, uint64_t rhs);
    bool rbt_verify();

protected:
    // return true if node is null
    bool is_null_node(uint64_t node);

    // ========== 纯虚函数 ========== //
    // 由于不同的struct node的结构不同，因此这些函数的具体实现也尽不相同
    // 故设置为纯虚函数，让子类去实现

    // 由于tree的删除是需要销毁节点的，但是数组模拟的树却不需要销毁节点，而是进行其他的操作
    // 由于这需要表现出差异性，因此被设为纯虚函数
    // return the rbt root node
    virtual uint64_t get_root() const = 0;
    virtual bool set_root(uint64_t new_root) = 0;

    // return true if successful destruct the node
    virtual bool destruct_node(uint64_t node) = 0;

    // return true if they are the same
    virtual bool is_nodes_equal(uint64_t first, uint64_t second) = 0;

    // return parent pointer as uint64_t
    virtual uint64_t get_parent(uint64_t node) = 0;
    // return true if successful set node parent as parent
    virtual bool set_parent(uint64_t node, uintptr_t parent) = 0;

    // return left child pointer as uint64_t
    virtual uint64_t get_left_child(uint64_t node) = 0;
    // return true if successful set node left child as left_child
    virtual bool set_left_child(uint64_t node, uint64_t left_child) = 0;

    // return right child pointer as uint64_t
    virtual uint64_t get_right_child(uint64_t node) = 0;
    // return true if successful set node right child as right_child
    virtual bool set_right_child(uint64_t node, uint64_t right_child) = 0;

    // return <rbt_color_t>: the color of the node
    virtual rbt_color_t get_color(uint64_t node) = 0;
    // return true if successful set node color as color
    virtual bool set_color(uint64_t node, rbt_color_t color) = 0;

    // return the node key
    virtual uint64_t get_key(uint64_t node) = 0;
    // return true successful set node key as key
    virtual bool set_key(uint64_t node, uint64_t key) = 0;

    // return the node value
    virtual uint64_t get_value(uint64_t node) = 0;
    // return true successful set node value as value
    virtual bool set_value(uint64_t node, uint64_t value) = 0;

private:
    // some function for helping rbt insert and delete
    bool bst_set_child(uint64_t parent, uint64_t child, child_t direction);
    void bst_replace(uint64_t victim, uint64_t node);
    void bst_insert_node(uint64_t node);

    uint64_t rbt_rotate(uint64_t node, uint64_t parent, uint64_t grandparent);

    void rbt_delete_node_only(uint64_t node, uint64_t &db_parent);
    void rbt_get_psnf(uint64_t db, uint64_t &parent, uint64_t &sibling, uint64_t &near, uint64_t &far);
};

// ================================================ //
//    The implementation of the default  RB-Tree    //
// ================================================ //
typedef struct RBT_INT_NODE {
    // pointers
    struct RBT_INT_NODE *parent = nullptr;
    struct RBT_INT_NODE *left = nullptr;
    struct RBT_INT_NODE *right = nullptr;

    // edge color to parent also as node color
    rbt_color_t color = COLOR_RED;

    // tree node key
    uint64_t key = 0;

    // tree node values
    uint64_t value = 0;

    // 构造函数
    RBT_INT_NODE() = default;
    RBT_INT_NODE(RBT_INT_NODE *p, RBT_INT_NODE *l, RBT_INT_NODE *r,
                 rbt_color_t c, uint64_t k, uint64_t v)
        : parent(p), left(l), right(r), color(c), key(k), value(v) {}
} rbt_node_t;

class RBT_INT final : public RBT{
public:
    RBT_INT(uint64_t root)
        : root_(root) {}

    RBT_INT(char *tree, char *color);

    ~RBT_INT() override {
        delete_rbt();
    }

protected:
    uint64_t get_root() const override;
    bool set_root(uint64_t new_root) override;

    bool destruct_node(uint64_t node) override;

    bool is_nodes_equal(uint64_t first, uint64_t second) override;

    uint64_t get_parent(uint64_t node) override;
    bool set_parent(uint64_t node, uintptr_t parent) override;

    uint64_t get_left_child(uint64_t node) override;
    bool set_left_child(uint64_t node, uint64_t left_child) override;

    uint64_t get_right_child(uint64_t node) override;
    bool set_right_child(uint64_t node, uint64_t right_child) override;

    rbt_color_t get_color(uint64_t node) override;
    bool set_color(uint64_t node, rbt_color_t color) override;

    uint64_t get_key(uint64_t node) override;
    bool set_key(uint64_t node, uint64_t key) override;

    uint64_t get_value(uint64_t node) override;
    bool set_value(uint64_t node, uint64_t value) override;

private:
    void delete_rbt();
    // 删除以root为根的rb-tree
    void delete_rbt(uint64_t root);

    uint64_t root_ = NULL_NODE;
};

#endif //MYMALLOC_RBT_H
