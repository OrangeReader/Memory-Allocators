#ifndef MALLOC_LINKED_LIST_H
#define MALLOC_LINKED_LIST_H
#include <cassert>
#include <cstdint>

/*======================================*/
/*      Circular Doubly Linked List     */
/*======================================*/
template<typename LINKED_LIST_NODE_T>
class LINKED_LIST {
public:
    // ========== 拷贝控制 ========== //
    LINKED_LIST(LINKED_LIST_NODE_T *head, uint64_t count)
        : head_(head), count_(count) {}

    // 拷贝构造、赋值

    // 移动构造、赋值

    // 继承体系中，我们只需要删除相应的成员即可
    virtual ~LINKED_LIST() {
        delete_list();
    }

    // ========== 通用函数 ========== //
    // 下面的纯虚函数屏蔽了struct node在底层结构上的差异
    // 通过调用下面的纯虚函数使得此部分提供的函数变得更加通用
    bool insert_node(LINKED_LIST_NODE_T *node);
    bool delete_node(LINKED_LIST_NODE_T *node);
    void delete_list();
    uint64_t count() const {
        return count_;
    }
    // for traverse the linked list
    LINKED_LIST_NODE_T *get_next();
    LINKED_LIST_NODE_T *get_node_by_index(uint64_t index);

    // ========== 纯虚函数 ========== //
    // 由于不同的struct node的结构不同，因此这些函数的具体实现也尽不相同
    // 故设置为纯虚函数，让子类去实现
    // return true if node is null
    virtual bool is_null_node(LINKED_LIST_NODE_T *node) = 0;

    // return linked list node
    virtual LINKED_LIST_NODE_T *construct_node() = 0;
    // return true if successful destruct the node
    virtual bool destruct_node(LINKED_LIST_NODE_T *node) = 0;

    // return true if they are the same
    virtual bool is_nodes_equal(LINKED_LIST_NODE_T *first, LINKED_LIST_NODE_T *second) = 0;

    // return the node of the previous node
    virtual LINKED_LIST_NODE_T *get_node_prev(LINKED_LIST_NODE_T *node) = 0;
    // return bool: true if the setting(node->prev = prev) is successful
    virtual bool set_node_prev(LINKED_LIST_NODE_T *node, LINKED_LIST_NODE_T *prev) = 0;

    // return the node of the next node
    virtual LINKED_LIST_NODE_T *get_node_next(LINKED_LIST_NODE_T *node) = 0;
    // return bool: true if the setting(node->next = next) is successful
    virtual bool set_node_next(LINKED_LIST_NODE_T *node, LINKED_LIST_NODE_T *next) = 0;

private:
    LINKED_LIST_NODE_T *head_ = nullptr;
    uint64_t count_ = 0;
};

// 模板的声明和实现最好放在同一个文件中
template<typename LINKED_LIST_NODE_T>
bool LINKED_LIST<LINKED_LIST_NODE_T>::insert_node(LINKED_LIST_NODE_T *node) {
    if (is_null_node(node)) {
        return false;
    }

    if (head_ == nullptr && count_ == 0) {
        // 当前是一个空链表
        // create a new head
        head_ = node;
        count_ = 1;

        // circular linked list initialization
        set_node_prev(node, node);
        set_node_next(node, node);

        return true;
    } else if (head_ != nullptr && count_ != 0) {
        // 当前链表不是空链表
        // 头插法: insert to head
        LINKED_LIST_NODE_T *head = head_;
        LINKED_LIST_NODE_T *head_prev = get_node_prev(head);

        set_node_next(node, head);
        set_node_prev(head, node);

        set_node_next(head_prev, node);
        set_node_prev(node, head_prev);

        head_ = node;
        ++count_;

        return true;
    } else {
        // 非法情况
        return false;
    }

}

template<typename LINKED_LIST_NODE_T>
bool LINKED_LIST<LINKED_LIST_NODE_T>::delete_node(LINKED_LIST_NODE_T *node) {
    if (head_ == nullptr || is_null_node(node)) {
        return false;
    }

    // update the prev and next pointers
    // !!! same for the only one node situation
    LINKED_LIST_NODE_T *prev = get_node_prev(node);
    LINKED_LIST_NODE_T *next = get_node_next(node);

    set_node_next(prev, next);
    set_node_prev(next, prev);

    // if this node to be free is the head
    if (is_nodes_equal(node, head_)) {
        head_ = next;
    }

    destruct_node(node);
    --count_;

    // 如果只有一个节点时
    if (count_ == 0) {
        head_ = nullptr;
    }

    return true;
}

template<typename LINKED_LIST_NODE_T>
LINKED_LIST_NODE_T *LINKED_LIST<LINKED_LIST_NODE_T>::get_next() {
    if (head_ == nullptr) {
        return nullptr;
    }

    LINKED_LIST_NODE_T *old_head = head_;
    head_ = get_node_next(old_head);

    return old_head;
}

template<typename LINKED_LIST_NODE_T>
LINKED_LIST_NODE_T *LINKED_LIST<LINKED_LIST_NODE_T>::get_node_by_index(uint64_t index) {
    // index in [0, count_)
    if (head_ == nullptr || index >= count_) {
        return nullptr;
    }

    LINKED_LIST_NODE_T *p = head_;
    for (int i = 0; i < index; ++i) {
        p = get_node_next(p);
    }

    return p;
}

template<typename LINKED_LIST_NODE_T>
void LINKED_LIST<LINKED_LIST_NODE_T>::delete_list() {
    // 在delete中，count_会变化
    // not multi-thread safe
    int count_copy = count_;
    for (int i = 0; i < count_copy; ++i) {
        LINKED_LIST_NODE_T *temp = get_next();
        delete_node(temp);
    }
}

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
    INT_LINKED_LIST_NODE(int v, struct INT_LINKED_LIST_NODE *p, struct INT_LINKED_LIST_NODE *n)
        : value(v), prev(p), next(n) {}
} int_linked_list_node_t;

class INT_LINKED_LIST final : public LINKED_LIST<int_linked_list_node_t>  {
public:
    // construct function
    INT_LINKED_LIST(int_linked_list_node_t *head, uint64_t count)
        : LINKED_LIST(head, count) {}

    // 派生类中没有自己的成员变量需要被管理，因此设置为default即可
    // 然后调用父类的析构函数释放相关内存
    ~INT_LINKED_LIST() final = default;

    // INT_LINKED_LIST中需要override的函数
    bool is_null_node(int_linked_list_node_t *node) override;

    int_linked_list_node_t * construct_node() override;
    bool destruct_node(int_linked_list_node_t *node) override;

    bool is_nodes_equal(int_linked_list_node_t *first, int_linked_list_node_t *second) override;

    int_linked_list_node_t * get_node_prev(int_linked_list_node_t *node) override;
    bool set_node_prev(int_linked_list_node_t *node, int_linked_list_node_t *prev) override;

    int_linked_list_node_t * get_node_next(int_linked_list_node_t *node) override;
    bool set_node_next(int_linked_list_node_t *node, int_linked_list_node_t *next) override;
};

#endif //MALLOC_LINKED_LIST_H