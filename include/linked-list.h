#ifndef MALLOC_LINKED_LIST_H
#define MALLOC_LINKED_LIST_H
#include <cassert>
#include <cstdint>

/*======================================*/
/*      Circular Doubly Linked List     */
/*======================================*/
// 为了能够拓展到后续的Explicit Free List，我们将链表节点的指针全部用uint64_t来表示
// 对于一个 uint64_t node
// 使用时将其类型转换 (LIST_NODE_TYPE *)node
// 构造后将其转为uint64_t即可
// 1. uint64_t node = (uint64_t)new LIST_NODE_TYPE
// 2. LIST_NODE_TYPE * node_ptr = new LIST_NODE_TYPE
//    node_ptr是一个指针，其实际上是一个uint64_t的value，表示节点的地址
//    uint64_t node = (uint64_t)node_ptr;
// 但是不可以对 `(uint64_t)node_ptr`赋值，需要 `*(uint64_t *)&node_ptr` 这样赋值
const int NULL_NODE = 0;

class LINKED_LIST {
public:
    // ========== 拷贝控制 ========== //
    LINKED_LIST() = default;

    // 继承体系中，我们只需要删除相应的成员即可
    // 因此，虽然析构函数都是调用delete_list私有成员函数
    // delete_list调用其余纯虚函数提供的接口来实现，可以屏蔽下层数据结构所带来的差异
    // ⭐⭐⭐ 但是运行到基类时，子类已经不存在了，因此调用的是基类的delete_list以及基类的纯虚函数，这是错误的！！！
    virtual ~LINKED_LIST() = default;

    // ========== 通用函数 ========== //
    // 下面的纯虚函数屏蔽了struct node在底层结构上的差异
    // 通过调用下面的纯虚函数使得此部分提供的函数变得更加通用
    bool insert_node(uint64_t node);
    bool delete_node(uint64_t node);
    uint64_t count() const {
        return get_count();
    }

    uint64_t head() const {
        return get_head();
    }

    uint64_t get_next_node(uint64_t node) {
        return get_node_next(node);
    }
    uint64_t get_prev_node(uint64_t node) {
        return get_node_prev(node);
    }

    // for traverse the linked list
    uint64_t get_next();
    uint64_t get_node_by_index(uint64_t index);

protected:
    // return true if node is null
    bool is_null_node(uint64_t node);

    // ========== 纯虚函数 ========== //
    // 由于不同的struct node的结构不同，因此这些函数的具体实现也尽不相同
    // 故设置为纯虚函数，让子类去实现
    // return header
    virtual uint64_t get_head() const = 0;
    // return true if successful set head
    virtual bool set_head(uint64_t new_head) = 0;

    // return linked list length
    virtual uint64_t get_count() const = 0;
    // return true if successful set count
    virtual bool set_count(uint64_t new_count) = 0;

    // return true if successful destruct the node
    virtual bool destruct_node(uint64_t node) = 0;

    // return true if they are the same
    virtual bool is_nodes_equal(uint64_t first, uint64_t second) = 0;

    // return the node of the previous node
    virtual uint64_t get_node_prev(uint64_t node) = 0;
    // return bool: true if the setting(node->prev = prev) is successful
    virtual bool set_node_prev(uint64_t node, uint64_t prev) = 0;

    // return the node of the next node
    virtual uint64_t get_node_next(uint64_t node) = 0;
    // return bool: true if the setting(node->next = next) is successful
    virtual bool set_node_next(uint64_t node, uint64_t next) = 0;
};

// ================================================ //
//   The implementation of the default linked list  //
// ================================================ //

// node value is int
typedef struct INT_LINKED_LIST_NODE {
    int value = 0;
    struct INT_LINKED_LIST_NODE *prev = nullptr;
    struct INT_LINKED_LIST_NODE *next = nullptr;

    // 构造函数
    INT_LINKED_LIST_NODE() = default;   // 默认构造
    INT_LINKED_LIST_NODE(int v) : value(v) {}
} int_linked_list_node_t;

class INT_LINKED_LIST final : public LINKED_LIST  {
public:
    // construct function
    INT_LINKED_LIST(uint64_t head, uint64_t count)
        : head_(head), count_(count) {}

    // 派生类中没有自己的成员变量需要被管理，因此设置为default即可
    // 然后调用父类的析构函数释放相关内存
    ~INT_LINKED_LIST() override {
        delete_list();
    };

protected:
    // INT_LINKED_LIST中需要override的函数
    inline uint64_t get_head() const override;
    inline bool set_head(uint64_t new_head) override;

    inline uint64_t get_count() const override;
    inline bool set_count(uint64_t new_count) override;

    bool destruct_node(uint64_t node) override;

    bool is_nodes_equal(uint64_t first, uint64_t second) override;

    uint64_t get_node_prev(uint64_t node) override;
    bool set_node_prev(uint64_t node, uint64_t prev) override;

    uint64_t get_node_next(uint64_t node) override;
    bool set_node_next(uint64_t node, uint64_t next) override;

private:
    void delete_list();

    uint64_t head_;
    uint64_t count_;
};

#endif //MALLOC_LINKED_LIST_H