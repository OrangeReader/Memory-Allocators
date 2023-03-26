#include "linked-list.h"

/*======================================*/
/*      Default Implementation          */
/*======================================*/
bool INT_LINKED_LIST::is_null_node(int_linked_list_node_t *node) {
    if (node == nullptr) {
        return true;
    }
    return false;
}

int_linked_list_node_t *INT_LINKED_LIST::construct_node() {
    return new int_linked_list_node_t;
}

bool INT_LINKED_LIST::destruct_node(int_linked_list_node_t *node) {
    if (is_null_node(node)) {
        return false;
    }

    delete node;
    return true;
}

bool INT_LINKED_LIST::is_nodes_equal(int_linked_list_node_t *first, int_linked_list_node_t *second) {
    return first == second;
}

int_linked_list_node_t *INT_LINKED_LIST::get_node_prev(int_linked_list_node_t *node) {
    if (is_null_node(node)) {
        return nullptr;
    }

    return node->prev;
}

bool INT_LINKED_LIST::set_node_prev(int_linked_list_node_t *node, int_linked_list_node_t *prev) {
    if (is_null_node(node) || is_null_node(prev)) {
        return false;
    }

    node->prev = prev;
    return true;
}

int_linked_list_node_t *INT_LINKED_LIST::get_node_next(int_linked_list_node_t *node) {
    if (is_null_node(node)) {
        return nullptr;
    }
    return node->next;
}

bool INT_LINKED_LIST::set_node_next(int_linked_list_node_t *node, int_linked_list_node_t *next) {
    if (is_null_node(node) || is_null_node(next)) {
        return false;
    }

    node->next = next;
    return true;
}