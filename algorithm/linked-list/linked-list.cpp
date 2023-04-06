#include "linked-list.h"
#include <cstdio>
/*======================================*/
/*      Base class Implementation       */
/*======================================*/
bool LINKED_LIST::insert_node(uint64_t node) {
    if (is_null_node(node)) {
        return false;
    }

    if (head_ == NULL_NODE && count_ == 0) {
        // 当前是一个空链表
        // create a new head
        head_ = node;
        count_ = 1;

        // circular linked list initialization
        set_node_prev(node, node);
        set_node_next(node, node);

        return true;
    } else if (head_ != NULL_NODE && count_ != 0) {
        // 当前链表不是空链表
        // 头插法: insert to head
        uint64_t head = head_;
        uint64_t head_prev = get_node_prev(head);

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


bool LINKED_LIST::delete_node(uint64_t node) {
    if (head_ == NULL_NODE || is_null_node(node)) {
        return false;
    }

    // update the prev and next pointers
    // !!! same for the only one node situation
    uint64_t prev = get_node_prev(node);
    uint64_t next = get_node_next(node);

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
        head_ = NULL_NODE;
    }

    return true;
}

uint64_t LINKED_LIST::get_next() {
    if (head_ == NULL_NODE) {
        return NULL_NODE;
    }

    uint64_t old_head = head_;
    head_ = get_node_next(old_head);

    return old_head;
}

uint64_t LINKED_LIST::get_node_by_index(uint64_t index) {
    // index in [0, count_)
    if (head_ == NULL_NODE || index >= count_) {
        return NULL_NODE;
    }

    uint64_t p = head_;
    for (int i = 0; i < index; ++i) {
        p = get_node_next(p);
    }

    return p;
}

void LINKED_LIST::delete_list() {
    // 在delete中，count_会变化
    // not multi-thread safe
    int count_copy = count_;
    for (int i = 0; i < count_copy; ++i) {
        uint64_t temp = get_next();
        delete_node(temp);
    }
}

bool LINKED_LIST::is_null_node(uint64_t node) {
    if (node == NULL_NODE) {
        return true;
    }
    return false;
}

/*======================================*/
/*      Default Implementation          */
/*======================================*/

bool INT_LINKED_LIST::destruct_node(uint64_t node) {
    if (is_null_node(node)) {
        return false;
    }

    int_linked_list_node_t *temp = (int_linked_list_node_t *)node;
    delete temp;
    return true;
}

bool INT_LINKED_LIST::is_nodes_equal(uint64_t first, uint64_t second) {
    return first == second;
}

uint64_t INT_LINKED_LIST::get_node_prev(uint64_t node) {
    if (is_null_node(node)) {
        return NULL_NODE;
    }

    return uint64_t(((int_linked_list_node_t *)node)->prev);
}

bool INT_LINKED_LIST::set_node_prev(uint64_t node, uint64_t prev) {
    if (is_null_node(node) || is_null_node(prev)) {
        return false;
    }
    // prev 代表的是 一个节点的地址(uint64_t), 我们需要修改prev的值，让其指向我们的prev(uint64_t)
    // 可以单纯的解释为uint64_t，但是我们没有办法对其赋值：uint64_t(((int_linked_list_node_t *)node)->prev);
    // 为了能够赋值，我们需要取其地址，将其解释为uint64_t *，然后再取其值*

    ((int_linked_list_node_t *)node)->prev = (int_linked_list_node_t *)prev;
    return true;
}

uint64_t INT_LINKED_LIST::get_node_next(uint64_t node) {
    if (is_null_node(node)) {
        return NULL_NODE;
    }

    return uint64_t(((int_linked_list_node_t *)node)->next);
}

bool INT_LINKED_LIST::set_node_next(uint64_t node, uint64_t next) {
    if (is_null_node(node) || is_null_node(next)) {
        return false;
    }

    // prev 代表的是 一个节点的地址(uint64_t), 我们需要修改prev的值，让其指向我们的prev(uint64_t)
    // 为了将其解释为uint64_t，需要取其地址，将其解释为uint64_t *，然后再取其值
    ((int_linked_list_node_t *)node)->next = (int_linked_list_node_t *)next;
    return true;
}