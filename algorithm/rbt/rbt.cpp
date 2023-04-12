#include <cassert>
#include <cstring>
#include <cstdio>
#include "rbt.h"
#include "utils.h"

void RBT::insert_node(uint64_t node) {
    assert(is_null_node(node) == false);

    // set the inserted node as red node
    set_color(node, COLOR_RED);
    set_parent(node, NULL_NODE);
    set_left_child(node, NULL_NODE);
    set_right_child(node, NULL_NODE);

    // ⭐ 先当作bst进行插入，然后再调整，保证黑色完美平衡
    // ⭐ if tree is empty, x would be inserted as BLACK node
    bst_insert_node(node);

    // float up RBT
    uint64_t cur_node = node;
    while(true) {
        rbt_color_t node_color = get_color(cur_node);

        uint64_t node_parent = get_parent(cur_node);
        if (is_null_node(node_parent)) {
            // ⭐ end of floating up: ① RED NODE floating up to root, let root color as BLACK ==> BLACK height + 1
            set_color(cur_node, COLOR_BLACK);
            return ;
        } else {
            // 此处说明其插入后不是根节点，因此其必为RED NODE
            assert(node_color == COLOR_RED);

            rbt_color_t parent_color = get_color(node_parent);
            if (parent_color == COLOR_BLACK) {
                // ⭐ end of floating up: ② the red node which floating up, and it's parent are not both red
                return ;
            } else {
                // 插入的节点和父节点都是RED NODE，存在冲突，需要floating up
                // p is red && n is red
                // ==> g exists and its is black
                uint64_t node_grandparent = get_parent(node_parent);
                assert(is_null_node(node_grandparent) == false);
                assert(get_color(node_grandparent) == COLOR_BLACK);

                // rotate
                uint64_t root = rbt_rotate(cur_node, node_parent, node_grandparent);

                // recolor
                // 并不清楚node, parent 和 grandparent 哪一个作为旋转后的root
                set_color(cur_node, COLOR_BLACK);
                set_color(node_parent, COLOR_BLACK);
                set_color(node_grandparent, COLOR_BLACK);

                // ⭐ 将root作为RED NODE, floating up
                //  - 并不清楚grandparent另一个节点的颜色，因为grandparent为BLACK NODE，因此另一个孩子节点可能为红色节点
                //  - grandparent的key要么是最大，要么是最小，则其rotate后一定作为子节点，而非root
                //  - 因此如果将rotate后的孩子节点染成RED COLOR，那么RED NODE 不会floating up，
                //    而是有可能转移到了grandparent和其另一个孩子节点(可能为RED NODE)身上，则不好处理
                //    因此孩子节点为BLACK， root为RED，让RED NODE floating up，我们递归的去处理
                set_color(root, COLOR_RED);

                // 让root作为cur node，从而floating up
                cur_node = root;
                continue;
            }
        }
    }
}

void RBT::delete_node(uint64_t node) {
    uint64_t db = NULL_NODE;
    uint64_t parent = NULL_NODE;
    uint64_t sibling = NULL_NODE;
    uint64_t near = NULL_NODE;
    uint64_t far = NULL_NODE;

    rbt_delete_node_only(node, parent);

    // db can be root, then parent is null
    // no action would be taken for root double black
    // it will automatically turn to single black
    if (is_null_node(parent)) {
        return;
    }

    // after: parent can't be null, db can be null

    // re-balance the double black node
    while (is_nodes_equal(db, get_root())) {
        // to start up, db = NULL, p is effective
        // so the calculation will be on p instead of db
        rbt_get_psnf(db, parent, sibling, near, far);

        // n & f can be null, e.g.
        // (p, db, (s, n, f)) = (B, #, (B, #, #))
        // the color would be black for null
        rbt_color_t parent_color = get_color(parent);
        rbt_color_t sibling_color = get_color(sibling);
        rbt_color_t near_color = get_color(near);
        rbt_color_t far_color = get_color(far);

        // COLOR_RED = 0, COLOR_BLACK = 1
        int psnf_color = (parent_color << 3) | (sibling_color << 2) | (near_color << 1) | far_color;

        switch (psnf_color) {
            case 0XF:
                // parent, sibling, sibling's child are all black nodes
                // 此时无红色节点可以转移，则db上浮到父节点，设置其sibling为red node
                db = parent;
                set_color(sibling, COLOR_RED);

                // continue to float up (all possibilities)
                continue;
            case 0XB:
                // only sibling is the red node
                rbt_rotate(far, sibling, parent);

                // 抱持黑高不变，交换sibling和parent的color
                set_color(sibling, COLOR_BLACK);
                set_color(parent, COLOR_RED);
                // db is not changing, it can still be null
                // p is still the parent of db
                // continue to next iteration (0x4, 0x5, 0x6, 0x7)
                continue;
            case 0X7:
                // only parent is the red node
                // 此时将双黑节点中一个黑色信息转移至parent，为了保证黑高，将sibling节点标记为red，直接结束
                set_color(parent, COLOR_BLACK);
                set_color(sibling, COLOR_RED);
                break;
            case 0X4:
            case 0X5:
            case 0XC:
            case 0XD:
                // near is the red node(far adn parent may be red)
                // 将near节点旋转作为根节点
                rbt_rotate(near, sibling, parent);

                // 为了将修改影响局限于当前子树，则交换near和parent的color，对上层不产生影响
                set_color(sibling, parent_color);
                // 将双黑节点中一个黑色信息转移至parent(与near交换后是red color), 结束
                set_color(parent, COLOR_BLACK);
                break;
            case 0X6:
            case 0XE:
                // far is the red node, near is black node (parent may be red)
                rbt_rotate(far, sibling, parent);

                // 同样屏蔽对上层的影响，交换sibling(当前子树的root)与parent的color
                set_color(sibling, parent_color);
                set_color(parent, COLOR_BLACK);

                // 将双黑节点中一个黑色信息转移至far(red node)，结束
                set_color(far, COLOR_BLACK);
                break;
            default:
                assert(false);
                break;
        }
        // 走到这说明db节点已经处理完毕
        break;
    }

}

bool rbt_compare(uint64_t lhs, uint64_t rhs, const RBT *rbt) {
    bool is_lhs_null = rbt->is_null_node(lhs);
    bool is_rhs_null = rbt->is_null_node(rhs);

    if (is_lhs_null && is_rhs_null) {
        return true;
    }

    if (is_lhs_null || is_rhs_null) {
        return false;
    }

    // both not NULL
    if (rbt->get_key(lhs) == rbt->get_key(rhs)) {
        uint64_t lhs_parent = rbt->get_parent(lhs);
        uint64_t rhs_parent = rbt->get_parent(rhs);

        bool is_lhs_parent_null = rbt->is_null_node(lhs_parent);
        bool is_rhs_parent_null = rbt->is_null_node(rhs_parent);
        if (is_lhs_parent_null != is_rhs_parent_null) {
            return false;
        }

        if (is_lhs_parent_null == false) {
            if (rbt->get_key(lhs_parent) != rbt->get_key(rhs_parent)) {
                return false;
            }
        }
    }

    if (rbt->get_color(lhs) == rbt->get_color(rhs)) {
        return rbt_compare(rbt->get_left_child(lhs), rbt->get_left_child(rhs), rbt) &&
            rbt_compare(rbt->get_right_child(lhs), rbt->get_right_child(rhs), rbt);
    }

    return false;
}

bool RBT::is_null_node(uint64_t node) const {
    if (node == NULL_NODE) {
        return true;
    }

    return false;
}

bool RBT::bst_set_child(uint64_t parent, uint64_t child, child_t direction) {
    switch (direction) {
        case LEFT_CHILD:
            set_left_child(parent, child);
            if (!is_null_node(child)) {
                // 如果插入的孩子节点非空，则还需要设置其parent
                set_parent(child, parent);
            }
            break;
        case RIGHT_CHILD:
            set_right_child(parent,child);
            if (!is_null_node(child)) {
                set_parent(child, parent);
            }
            break;
        default:
            return false;
    }
    return true;
}

// ⭐ 使用node `替换` victim, victim节点中的信息仍然没有变(其左右孩子和parent依然是之前的)
//    但是parent现在已经无法找到它了，找到的而是node(⭐ 左右孩子仍然是不变的!!!)
// ⭐ 此时victim类似于删除，后续如何处理交由后续程序处理
void RBT::bst_replace(uint64_t victim, uint64_t node) {
    assert(is_null_node(victim) == false);
    assert(is_null_node(node) == false);

    uint64_t v_parent = get_parent(victim);
    if (is_nodes_equal(victim, get_root())) {
        // 如果被替换的是root节点
        assert(is_null_node(v_parent));

        set_root(node);
        set_parent(node, NULL_NODE);
        return;
    } else {
        // victim has parent
        uint64_t v_parent_left = get_left_child(v_parent);
        uint64_t v_parent_right = get_right_child(v_parent);

        if (is_nodes_equal(victim, v_parent_left)) {
            // victim is the left child of its parent
            bst_set_child(v_parent, node, LEFT_CHILD);
        } else {
            assert(is_nodes_equal(victim, v_parent_right));
            bst_set_child(v_parent, node, RIGHT_CHILD);
        }
    }
}

void RBT::bst_insert_node(uint64_t node) {
    assert(is_null_node(node) == false);

    uint64_t root = get_root();
    if (is_null_node(root)) {
        // tree is empty: let node as root
        set_parent(node, NULL_NODE);
        set_left_child(node, NULL_NODE);
        set_right_child(node, NULL_NODE);
        // rbt only
        set_color(node, COLOR_BLACK);

        set_root(node);
        return;
    }

    // tree is not empty: ⭐ search a `null place` and insert
    uint64_t node_key = get_key(node);
    while (!is_null_node(root)) {
        uint64_t root_key = get_key(root);
        if (node_key < root_key) {
            // 找到了第一个比插入节点key大的node
            uint64_t root_left = get_left_child(root);

            // 找到了插入位置
            if (is_null_node(root_left)) {
                bst_set_child(root, node, LEFT_CHILD);
                return;
            }

            // ⭐ 继续寻找，直到找到了一个合适的位置(null node)
            root = root_left;
        } else {
            // node_key >= root_key
            // ⭐ 为了方便起见，我们将bst的 left <= root <= right 改为 left < root <= right
            // 这样插入时，如果发现当前的遍历到的节点root和D相等，我们还得去看看左子树(是否相等，如果相等再去看其左子树和右子树)，然后再看看右子树
            // 修改后，我们发现相等时直接去右子树寻找，如果继续相等则继续去其右子树，否则就找到了一个合适的插入点
            uint64_t root_right= get_right_child(root);

            // 找到了插入位置
            if (is_null_node(root_right)) {
                bst_set_child(root, node, RIGHT_CHILD);
            }

            // ⭐ 继续寻找，直到找到了一个合适的位置(null node)
            root = root_right;
        }
    }
}

// 4 kinds of rotations
// return the new root of the subtree
uint64_t RBT::rbt_rotate(uint64_t node, uint64_t parent, uint64_t grandparent) {
    // MUST NOT be NULL
    assert(is_null_node(node) == false);
    assert(is_null_node(parent) == false);
    assert(is_null_node(grandparent) == false);

    // MUST be parent and grandparent
    assert(is_nodes_equal(parent, get_parent(node)));
    assert(is_nodes_equal(grandparent, get_parent(parent)));

    uint64_t node_left = get_left_child(node);
    uint64_t node_right = get_right_child(node);
    uint64_t parent_left = get_left_child(parent);
    uint64_t parent_right = get_right_child(parent);
    uint64_t grandparent_left = get_left_child(grandparent);

    if (is_nodes_equal(grandparent_left, parent)) {
        if (is_nodes_equal(parent_left, node)) {
            // (g,(p,(n,A,B),C),D) ==> (p,(n,A,B),(g,C,D))
            bst_replace(grandparent, parent);

            // ⭐ 此时g类似于被删除，处于游离状态
            // 0. g->parent, g->left_child 和 g->right_child 仍然没变!!!
            // 1. g->parent->left_child = p, 不再是g
            // 2. g->left_child->parent 和 g->right_child->parent 仍然是 g

            bst_set_child(grandparent, parent_right, LEFT_CHILD);
            bst_set_child(parent, grandparent, RIGHT_CHILD);
            return parent;
        } else {
            // (g,(p,A,(n,B,C)),D) ==> (n,(p,A,B),(g,C,D))
            bst_replace(grandparent, node);

            bst_set_child(parent, node_left, RIGHT_CHILD);
            bst_set_child(node, parent, LEFT_CHILD);
            bst_set_child(grandparent, node_right, LEFT_CHILD);
            bst_set_child(node, grandparent, RIGHT_CHILD);
            return node;
        }
    } else {
        if(is_nodes_equal(node, parent_left)) {
            // (g,A,(p,(n,B,C),D)) ==> (n,(g,A,B),(p,C,D))
            bst_replace(grandparent, node);

            bst_set_child(grandparent, node_left, RIGHT_CHILD);
            bst_set_child(node, grandparent, LEFT_CHILD);
            bst_set_child(parent, node_right, LEFT_CHILD);
            bst_set_child(node, parent, RIGHT_CHILD);
            return node;
        } else {
            // (g,A,(p,B,(n,C,D))) ==> (p,(g,A,B),(n,C,D))
            bst_replace(grandparent, parent);

            bst_set_child(grandparent, parent_left, RIGHT_CHILD);
            bst_set_child(parent, grandparent, LEFT_CHILD);
            return parent;
        }
    }
}

// 基本类似于bst的delete node，只是增加了考虑到双黑节点的处理
void RBT::rbt_delete_node_only(uint64_t node, uint64_t &db_parent) {
    db_parent = NULL_NODE;

    if (is_null_node(get_root())) {
        // nothing to delete
        return ;
    }

    if (is_null_node(node)) {
        // delete a null node
        return ;
    }

    uint64_t node_left = get_left_child(node);
    uint64_t node_right = get_right_child(node);

    bool is_node_left_null = is_null_node(node_left);
    bool is_node_right_null = is_null_node(node_right);

    if (is_node_left_null && is_node_right_null) {
        // case 1: leaf node: (x,#,#)
        // case 1.1: node is red, delete only
        // ⭐ case 1.2: node is black, general a double black node
        if (get_color(node) == COLOR_BLACK) {
            // after delete node that general a null node which is double black
            // report double black node to RBT to delete
            // then x is the root node, no db node and re-balancing
            db_parent = get_parent(node);
        }

        bst_replace(node, NULL_NODE);
        destruct_node(node);
        return;
    } else if (is_node_left_null || is_node_right_null) {
        // 如果两者都是null，则会命中第一个，则此处表达的是其中有一个为null，另一个不为null
        // case 2: only one null child
        //         (x,y,#) or (x,#,y)
        assert(get_color(node) == COLOR_BLACK);

        // the only non-null subtree: 其一定是红色节点(高度为1，但又非空)
        uint64_t red_child = NULL_NODE;
        if (is_node_left_null) {
            red_child = node_right;
        } else if (is_node_right_null) {
            red_child = node_left;
        } else {
            assert(false);
        }

        assert(get_color(red_child) == COLOR_RED);
        assert(is_null_node(get_left_child(red_child)));
        assert(is_null_node(get_right_child(red_child)));

        // 将其设置为黑色，然后顶替node
        set_color(red_child, COLOR_BLACK);
        bst_replace(node, red_child);
        destruct_node(node);
    } else {
        // case 3: no null child: (x,A,B)
        // check the node->right->left
        uint64_t node_right_left = get_left_child(node_right);
        bool is_node_right_left_null = is_null_node(node_right_left);

        uint64_t s = NULL_NODE;
        // swap the node and successor
        if (is_node_right_left_null) {
            // case 3.1: node->right is the successor
            // (x,A,(s,#,C))
            s = node_right;

            // 进行swap, 但 color 不变
            bst_set_child(node, get_right_child(s), RIGHT_CHILD);
            bst_set_child(node, NULL_NODE, LEFT_CHILD);

            bst_replace(node, s);

            bst_set_child(s, node_left, LEFT_CHILD);
            bst_set_child(s, node, RIGHT_CHILD);
        } else {
            // case 3.2: node.right.left....left is the successor
            s = node_right;
            uint64_t s_left = get_left_child(s);
            while(!is_null_node(s_left)) {
                s = s_left;
                s_left = get_left_child(s_left);
            }

            uint64_t s_parent = get_parent(s);
            // 进行swap, 但 color 不变
            bst_set_child(node, NULL_NODE, LEFT_CHILD);
            bst_set_child(node, get_right_child(s), RIGHT_CHILD);

            bst_replace(node, s);
            bst_set_child(s, node_left,LEFT_CHILD);
            bst_set_child(s, node_right, RIGHT_CHILD);

            bst_set_child(s_parent, node, LEFT_CHILD);
        }

        // ⭐⭐⭐ 保持color不变：仅仅违背了bst的性质，但是rbt的黑高和color没有变换
        rbt_color_t node_color = get_color(node);
        set_color(node, get_color(s));
        set_color(s, node_color);

        // ⭐ switch to case 1 or case 2
        assert(!is_null_node(node));
        // 转换后被删除的节点的左孩子一定是空节点
        assert(is_null_node(get_left_child(node)));

        rbt_delete_node_only(node, db_parent);
        return;
    }
}

// 只有db节点是root节点时，没有任何操作，否则相应的参数应该有正确的返回值
void RBT::rbt_get_psnf(uint64_t db, uint64_t &parent, uint64_t &sibling, uint64_t &near, uint64_t &far) {
    // db       -   double black node
    // parent   -   parent of db
    // sibling  -   sibling of db
    // near     -   the child of sibling. this child is near to db. BFS: (db, near, far) or (far, near, db)
    // far      -   the child of sibling. this child is far away from db
    if (is_null_node(db)) {
        if (is_null_node(parent)) {
            // db是空节点，但是parent同样也是空节点
            // ① ⭐下面的case 1，且删除的节点是root节点，此时无需设置psnf(一般不会进入这个选项)
            return;
        }
        // else:
        // ⭐⭐⭐ parent is effective, use parent to calculate s, n, f
        // this is when db is null. it can be 2 cases:
        //      1.  just called from bst delete, the db will be null
        //      2.  just from bst delete, and is case 0xB, after rotation,
        //          the db is still null (the old db)
    } else {
        // when db is not null, it's floating up
        parent = get_parent(db);
    }

    // ② ⭐db node floating up to root，此时仍然是无需设置psnf
    if (is_null_node(parent)) {
        // current double black node is the root of tree
        assert(is_null_node(db) == false);
        assert(is_nodes_equal(db, get_root()));

        return;
    }

    // now get parent node(not null)
    uint64_t parent_left = get_left_child(parent);
    uint64_t parent_right = get_right_child(parent);
    child_t db4parent = LEFT_CHILD;

    // this calculation is correct for (db == NULL) case
    if (is_nodes_equal(db, parent_left)) {
        sibling = parent_right;
        db4parent = LEFT_CHILD;
    } else {
        assert(is_nodes_equal(db, parent_right));
        sibling = parent_left;
        db4parent = RIGHT_CHILD;
    }

    assert(is_null_node(sibling) == false);

    uint64_t sibling_left = get_left_child(sibling);
    uint64_t sibling_right = get_right_child(sibling);
    if (db4parent) {
        // (p, db, (s, n, f))
        near = sibling_left;
        far = sibling_right;
    } else {
        // (p, (s, f, n), db)
        near = sibling_right;
        far = sibling_left;
    }
    // n & f can be null, e.g.:
    // (p, db, (s, n, f)) = (B, #, (B, #, #))
}

/*======================================*/
/*      Default Implementation          */
/*======================================*/
RBT_INT::RBT_INT(const char *tree, const char *color) {
    bst_construct_key_str(tree);

    // 空rbt，无需后续的染色
    if (is_nodes_equal(root_, NULL_NODE)) {
        return;
    }

    // root is not NULL
    // color the red-black tree
    int index = color_rbt_dfs(root_, color, 0);
    assert(index == strlen(color) - 1);
}

RBT_INT::RBT_INT(const RBT_INT &rhs) {
    // 遍历rbt，深拷贝一份
}

RBT_INT &RBT_INT::operator=(const RBT_INT &rhs) {
    // 遍历rbt，深拷贝一份
    return *this;
}

RBT_INT::RBT_INT(RBT_INT &&rhs) {
    root_ = rhs.get_root();

    // 记得将原有的root设置为空，否则会将rbt给析构掉
    rhs.set_root(NULL_NODE);
}

RBT_INT &RBT_INT::operator=(RBT_INT &&rhs) {
    root_ = rhs.get_root();

    // 记得将原有的root设置为空，否则会将rbt给析构掉
    rhs.set_root(NULL_NODE);
    return *this;
}

uint64_t RBT_INT::get_root() const {
    return root_;
}

bool RBT_INT::set_root(uint64_t new_root) {
    root_ = new_root;
    return true;
}

// for construct by str function
uint64_t RBT_INT::construct_node() {
    rbt_node_t *node = new RBT_INT_NODE();
    return (uint64_t)node;
}

bool RBT_INT::destruct_node(uint64_t node) {
    if (is_null_node(node)) {
        return false;
    }

    rbt_node_t *ptr = (rbt_node_t *)node;
    delete ptr;

    return true;
}

bool RBT_INT::is_nodes_equal(uint64_t first, uint64_t second) {
    return first == second;
}

uint64_t RBT_INT::get_parent(uint64_t node) const {
    if (is_null_node(node)) {
        return NULL_NODE;
    }

    return (uint64_t)(((rbt_node_t *)node)->parent);
}

bool RBT_INT::set_parent(uint64_t node, uintptr_t parent) {
    if (is_null_node(node)) {
        return false;
    }

    ((rbt_node_t *)node)->parent = (rbt_node_t *)parent;
    return true;
}

uint64_t RBT_INT::get_left_child(uint64_t node) const {
    if (is_null_node(node)) {
        return NULL_NODE;
    }

    return (uint64_t)(((rbt_node_t *)node)->left);
}

bool RBT_INT::set_left_child(uint64_t node, uint64_t left_child) {
    if (is_null_node(node)) {
        return false;
    }

    ((rbt_node_t *)node)->left = (rbt_node_t *)left_child;
    return true;
}

uint64_t RBT_INT::get_right_child(uint64_t node) const {
    if (is_null_node(node)) {
        return NULL_NODE;
    }

    return (uint64_t)(((rbt_node_t *)node)->right);
}

bool RBT_INT::set_right_child(uint64_t node, uint64_t right_child) {
    if (is_null_node(node)) {
        return false;
    }

    ((rbt_node_t *)node)->right = (rbt_node_t *)right_child;
    return true;
}

rbt_color_t RBT_INT::get_color(uint64_t node) const {
    if (is_null_node(node)) {
        // 对于空节点返回黑色
        return COLOR_BLACK;
    }
    return ((rbt_node_t *)node)->color;
}

bool RBT_INT::set_color(uint64_t node, rbt_color_t color) {
    if (is_null_node(node)) {
        return false;
    }

    ((rbt_node_t *)node)->color = color;
    return true;
}

uint64_t RBT_INT::get_key(uint64_t node) const {
    if (is_null_node(node)) {
        return NULL_NODE;
    }

    return ((rbt_node_t *)node)->key;
}

bool RBT_INT::set_key(uint64_t node, uint64_t key) {
    if (is_null_node(node)) {
        return false;
    }

    ((rbt_node_t *)node)->key = key;
    return true;
}

uint64_t RBT_INT::get_value(uint64_t node) const {
    if (is_null_node(node)) {
        return NULL_NODE;
    }

    return ((rbt_node_t *)node)->value;
}

bool RBT_INT::set_value(uint64_t node, uint64_t value) {
    if (is_null_node(node)) {
        return false;
    }

    ((rbt_node_t *)node)->value = value;
    return true;
}

// the format of string str:
// 1. NULL node - `#`
// 2. (root node key, left tree key, right tree key)
void RBT_INT::bst_construct_key_str(const char *str) {
    // 接下来使用栈(使用数组+top指针模拟)来处理节点，从而完成bst的构造
    // ⭐ a node on STACK, a LOCAL variable!(处于栈中的节点，说明其还未处理完)
    // this is the sentinel to mark the unprocessed subtree
    // ==> this node_id is different from NULL_ID, and should be different from all possibilities from node constructor

    const uint64_t todo = 1;    // 表示该subtree还没构造完全

    // pointer stack (at most 1K nodes) for node ids
    uint64_t stack[1000];
    int top = -1;

    int i = 0;
    while (i < strlen(str)) {
        if (str[i] == '(') {
            // push the node as being processed
            ++top;

            uint64_t x = construct_node();
            set_parent(x, NULL_NODE);
            set_left_child(x, todo);
            set_right_child(x, todo);

            // scan the value
            // (value,
            int j = i + 1;
            while ('0' <= str[j] && str[j] <= '9') {
                ++j;
            }
            set_key(x, string2uint_range(str, i + 1, j - 1));

            // push to stack
            stack[top] = x;

            // move to next node
            i = j + 1;  // 越过`,`
            continue;
        } else if (str[i] == ')') {
            // 说明当前的左右子树构造完成
            // pop the being processed node
            if (top == 0) {
                // pop root
                uint64_t root = stack[0];
                uint64_t root_left = get_left_child(root);
                uint64_t root_right = get_right_child(root);

                assert(is_nodes_equal(root_left, todo) == false);
                assert(is_nodes_equal(root_right, todo) == false);

                set_root(root);
                return;
            }

            // pop a non-root node
            uint64_t p = stack[top - 1];    // the parent of top
            uint64_t t = stack[top];        // the poped top - can be left or right child of parent

            // when pop top, its left & right subtree are all reduced(该节点的左右子树已经规约好了)
            assert(is_nodes_equal(get_left_child(t), todo) == false &&
                   is_nodes_equal(get_right_child(t), todo) == false);

            --top;

            // pop this node
            uint64_t p_left = get_left_child(p);
            uint64_t p_right = get_right_child(p);

            if (is_nodes_equal(p_left, todo)) {
                // left child of parent not yet processed
                bst_set_child(p, t, LEFT_CHILD);
                ++i;
                continue;
            } else if (is_nodes_equal(p_right, todo)) {
                // right child of parent not yet processed
                bst_set_child(p, t, RIGHT_CHILD);
                ++i;
                continue;
            }
            printf("node %lx:%lx is not having any unprocessed sub-tree\n  while %lx:%lx is redblack into it.\n",
                   p, get_key(p), t, get_key(t));
            assert(false);
        } else if (str[i] == '#') {
            if (top < 0) {
                // 一个空rbt
                assert(strlen(str) == 1);
                return;
            }

            uint64_t t = stack[top];    // pop 就是这个空节点的父节点

            // push NULL node
            // pop NULL node
            if (is_nodes_equal(get_left_child(t), todo)) {
                // left child of parent not yet processed
                bst_set_child(t, NULL_NODE, LEFT_CHILD);
                ++i;
                continue;
            } else if (is_nodes_equal(get_right_child(t), todo)) {
                // right child of parent not yet processed
                bst_set_child(t, NULL_NODE, RIGHT_CHILD);
                ++i;
                continue;
            }
            printf("node %lx:(%lx) is not having any unprocessed sub-tree\n  while NULL is redblack into it.\n",
                   t, get_key(t));
            assert(false);
        } else {
            // space, comma, new line
            ++i;
            continue;
        }
    }
}

uint64_t RBT_INT::color_rbt_dfs(uint64_t node, const char *color, int index) {
    if (is_null_node(node)) {
        assert(color[index] == '#');
        return index;
    }

    if (color[index] == 'R') {
        set_color(node, COLOR_RED);
    } else if (color[index] == 'B') {
        set_color(node, COLOR_BLACK);
    }

    // 获取当前已经处理到color的index
    index = color_rbt_dfs(get_left_child(node), color, index + 1);
    // 遍历右子树，返回的是color的最后一位的index
    index = color_rbt_dfs(get_right_child(node), color, index + 1);

    return index;
}


// for destruct function
void RBT_INT::delete_rbt() {
    if (is_null_node(root_)) {
        return ;
    }

    delete_rbt(root_);
}

void RBT_INT::delete_rbt(uint64_t root) {
    if (is_null_node(root)) {
        return ;
    }

    // (sub)root is not null
    // free its left and right child
    delete_rbt(get_left_child(root));
    delete_rbt(get_right_child(root));

    // free the (sub)root node
    destruct_node(root);
}
