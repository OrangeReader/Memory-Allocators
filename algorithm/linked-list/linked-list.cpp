#include "linked-list.h"

/*======================================*/
/*      Base class Implementation       */
/*======================================*/
bool LINKED_LIST::insert_node(uint64_t node) {
    uint64_t cur_head = get_head();
    uint64_t cur_count = get_count();

    if (is_null_node(node)) {
        return false;
    }

    if (cur_head == NULL_LIST_NODE && cur_count == 0) {
        // 当前是一个空链表
        // create a new head
        set_head(node);
        set_count(1);

        // circular linked list initialization
        set_node_prev(node, node);
        set_node_next(node, node);

        return true;
    } else if (cur_head != NULL_LIST_NODE && cur_count != 0) {
        // 当前链表不是空链表
        // 头插法: insert to head
        uint64_t head_prev = get_node_prev(cur_head);

        set_node_next(node, cur_head);
        set_node_prev(cur_head, node);

        set_node_next(head_prev, node);
        set_node_prev(node, head_prev);

        set_head(node);
        set_count(cur_count + 1);

        return true;
    } else {
        // 非法情况
        return false;
    }
}


bool LINKED_LIST::delete_node(uint64_t node) {
    uint64_t cur_head = get_head();
    uint64_t cur_count = get_count();

    if (cur_head == NULL_LIST_NODE || is_null_node(node)) {
        return false;
    }

    // update the prev and next pointers
    // !!! same for the only one node situation
    uint64_t prev = get_node_prev(node);
    uint64_t next = get_node_next(node);

    set_node_next(prev, next);
    set_node_prev(next, prev);

    // if this node to be free is the head
    if (is_nodes_equal(node, cur_head)) {
        set_head(next);
    }

    destruct_node(node);

    --cur_count;
    set_count(cur_count);

    // 如果只有一个节点时
    if (cur_count == 0) {
        set_head(NULL_LIST_NODE);
    }

    return true;
}

uint64_t LINKED_LIST::get_next() {
    uint64_t cur_head = get_head();

    if (cur_head == NULL_LIST_NODE) {
        return NULL_LIST_NODE;
    }

    uint64_t new_head = get_node_next(cur_head);
    set_head(new_head);

    return cur_head;
}

uint64_t LINKED_LIST::get_node_by_index(uint64_t index) {
    uint64_t cur_head = get_head();
    uint64_t cur_count = get_count();

    // index in [0, cur_count)
    if (cur_head == NULL_LIST_NODE || index >= cur_count) {
        return NULL_LIST_NODE;
    }

    for (int i = 0; i < index; ++i) {
        cur_head = get_node_next(cur_head);
    }

    return cur_head;
}

bool LINKED_LIST::is_null_node(uint64_t node) {
    if (node == NULL_LIST_NODE) {
        return true;
    }
    return false;
}

/*======================================*/
/*      Default Implementation          */
/*======================================*/

uint64_t INT_LINKED_LIST::get_head() const {
    return head_;
}

bool INT_LINKED_LIST::set_head(uint64_t new_head) {
    head_ = new_head;
    return true;
}

uint64_t INT_LINKED_LIST::get_count() const {
    return count_;
}

bool INT_LINKED_LIST::set_count(uint64_t new_count) {
    count_ = new_count;
    return true;
}

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
        return NULL_LIST_NODE;
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
        return NULL_LIST_NODE;
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

void INT_LINKED_LIST::delete_list() {
    // 在delete中，count_会变化
    // not multi-thread safe
    int count_copy = count();
    for (int i = 0; i < count_copy; ++i) {
        uint64_t temp = get_next();
        delete_node(temp);
    }
}