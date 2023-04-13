#include <cstdio>
#include <cassert>
#include <memory>
#include <vector>

#include "rbt.h"

using namespace std;

void rbt_verify_dfs(const rbt_node_t *root, uint64_t &black_height, uint64_t &key_min, uint64_t &key_max) {
    // 递归出口
    if (root == nullptr) {
        black_height = 1;               // 空节点认为黑高为1
        // 下面可设可不设，后续起始不会使用
        key_min = 0XFFFFFFFFFFFFFFFF;
        key_max = 0XFFFFFFFFFFFFFFFF;
        return;
    }

    rbt_color_t root_color = root->color;
    uint64_t root_key = root->key;

    rbt_node_t *left = root->left;
    uint64_t left_bh = 0XFFFFFFFFFFFFFFFF;
    uint64_t left_key_min = 0XFFFFFFFFFFFFFFFF;
    uint64_t left_key_max = 0XFFFFFFFFFFFFFFFF;
    rbt_verify_dfs(left, left_bh, left_key_min, left_key_max);

    rbt_node_t *right = root->right;
    uint64_t right_bh = 0XFFFFFFFFFFFFFFFF;
    uint64_t right_key_min = 0XFFFFFFFFFFFFFFFF;
    uint64_t right_key_max = 0XFFFFFFFFFFFFFFFF;
    rbt_verify_dfs(right, right_bh, right_key_min, right_key_max);

    // check color and black height
    assert(left_bh == right_bh);
    if (root_color == COLOR_BLACK) {
        black_height = left_bh + 1;
    } else if (root_color == COLOR_RED) {
        // 如果是红色节点，那么其孩子节点一定是黑色
        assert(left == nullptr || left->color == COLOR_BLACK);
        assert(right == nullptr || right->color == COLOR_BLACK);
        black_height = left_bh;
    } else {
        assert(false);
    }

    // check key
    // 非空节点，默认将其区间设置为其key
    key_min = root_key;
    key_max = root_key;
    if (left) {
        // ⭐ 虽然我们在bst中保证了 `left_max < root <= right_min`(bst中一定成立，可以不平衡)
        // 但是在rbt中，我们为了保证相对平衡
        // ⭐ 在当 插入时避免父子节点同为红色节点 以及 删除时消除双黑节点 所做的旋转操作也会让这个性质被破坏
        assert(left_key_max <= root_key);
        key_min = left_key_min;
    }

    if (right) {
        assert(root_key <= right_key_min);
        key_max = right_key_max;
    }

    return;
}

void rbt_verify(const shared_ptr<RBT_INT> rbt) {
    if (rbt == nullptr) {
        return ;
    }

    rbt_node_t *root = (rbt_node_t *)rbt->get_root();
    if (root == nullptr) {
        return ;
    }

    uint64_t tree_bh = 0;
    uint64_t tree_min = 0xFFFFFFFFFFFFFFFF;
    uint64_t tree_max = 0xFFFFFFFFFFFFFFFF;

    rbt_verify_dfs(root, tree_bh, tree_min, tree_max);
}

bool rbt_compare(const std::shared_ptr<RBT> lhs, const std::shared_ptr<RBT> rhs) {
    if (lhs == nullptr && rhs == nullptr) {
        return true;
    }

    if (lhs == nullptr || rhs == nullptr) {
        // 由于两者全部为空时走上面条件判断，因此此时对应的是两者有一个为空
        return false;
    }

    // 仅仅作为接口传入进去，用于访问RBT中的相关函数
    shared_ptr<RBT_INT> interface = make_shared<RBT_INT>(NULL_NODE);
    bool res = rbt_compare(lhs->get_root(), rhs->get_root(), interface);

    return res;
}

// rotation test
// if you want run this function, you should do
//  1. set rotate function which in RBT class as public
//  2. open a not correct rbt_int construct function in RBT_INT class
//static void test_rotation() {
//    printf("Testing Red-Black tree rotation ...\n");
//
//    RBT_INT *t = nullptr;
//    RBT_INT *ans = nullptr;
//    char inputs[8][100] = {
//            // grandparent is root
//            // case 1: (g, p, n): left, left
//            "(6," // grandparent
//            "(4," // parent
//            "(2," // node
//            "(1,#,#),(3,#,#)),(5,#,#)),(7,#,#))",
//            // case 2: (g, p, n): left, right
//            "(6," // g
//            "(2," // p
//            "(1,#,#),"
//            "(4," // n
//            "(3,#,#),(5,#,#))),(7,#,#))",
//            // case 3: (g, p, n): right, left
//            "(2," // g
//            "(1,#,#),"
//            "(6," // p
//            "(4," // n
//            "(3,#,#),(5,#,#)),(7,#,#)))",
//            // case 4: (g, p, n): right, right
//            "(2," // g
//            "(1,#,#),"
//            "(4," // p
//            "(3,#,#),"
//            "(6," // n
//            "(5,#,#),(7,#,#))))",
//
//            // grandparent is not root, is root->right
//            // case 1: (g, p, n): left, left
//            "(0,#,"
//            "(6," // g
//            "(4," // p
//            "(2," // g
//            "(1,#,#),(3,#,#)),(5,#,#)),(7,#,#)))",
//            // case 2: (g, p, n): left, right
//            "(0,#,"
//            "(6," // g
//            "(2," // p
//            "(1,#,#),"
//            "(4," // n
//            "(3,#,#),(5,#,#))),(7,#,#)))",
//            // case 3: (g, p, n): right, left
//            "(0,#,"
//            "(2," // g
//            "(1,#,#),"
//            "(6," // p
//            "(4," // n
//            "(3,#,#),(5,#,#)),(7,#,#))))",
//            // case 4: (g, p, n): right, right
//            "(0,#,"
//            "(2," // g
//            "(1,#,#),"
//            "(4," // p
//            "(3,#,#),"
//            "(6," // n
//            "(5,#,#),(7,#,#)))))",
//    };
//
//    char ans_grandparent_is_root[100] = "(4,(2,(1,#,#),(3,#,#)),(6,(5,#,#),(7,#,#)))";
//    char ans_grandparent_not_root[100] = "(0,#,(4,(2,(1,#,#),(3,#,#)),(6,(5,#,#),(7,#,#))))";
//
//    rbt_node_t *grandparent = nullptr;
//    rbt_node_t *parent = nullptr;
//    rbt_node_t *node = nullptr;
//
//    for (int i = 0; i < 8; ++i) {
//        t = new RBT_INT(inputs[i]);
//
//        // get grandparent
//        if (((i >> 2) & 0x1) == 0) {
//            // test grandparent root
//            // inputs[0] ~ inputs[3]
//            grandparent = (rbt_node_t *)t->get_root();
//        } else {
//            // test grandparent not root, is root->right
//            // inputs[4] ~ inputs[7]
//            grandparent = ((rbt_node_t *)t->get_root())->right;
//        }
//
//        // get grandparent
//        if (((i >> 1) & 0x1) == 0) {
//            // inputs[i], i is even
//            parent = grandparent->left;
//        } else {
//            // i is odd
//            parent = grandparent->right;
//        }
//
//        // get node
//        if ((i & 0x1) == 0) {
//            node = parent->left;
//        } else {
//            node = parent->right;
//        }
//
//        // rotate 需要获取当前操作的红黑树的 root，因此传入的是旋转操作的tree
//        t->rbt_rotate((std::uint64_t)node, (std::uint64_t)parent, (std::uint64_t)grandparent);
//        if (i < 4) {
//            ans = new RBT_INT(ans_grandparent_is_root);
//        } else {
//            ans = new RBT_INT(ans_grandparent_not_root);
//        }
//
//        assert(rbt_compare(t, ans));
//
//        delete t;
//        t = nullptr;
//
//        delete ans;
//        ans = nullptr;
//    }
//
//    printf("\033[32;1m\tPass\033[0m\n");
//}

static void test_insert() {
    printf("Testing Red-Black tree insertion ...\n");
    std::shared_ptr<RBT_INT> r = make_shared<RBT_INT>(
            "(11,"
                    "(2,"
                        "(1,#,#),"
                        "(7,"
                            "(5,#,#),"
                            "(8,#,#)"
                        ")"
                    "),"
                    "(14,#,(15,#,#))"
            ")",
            "B"
                    "R"
                        "B##"
                        "B"
                            "R##"
                            "R##"
                    "B#R##");

    // test insert
    rbt_verify(r);
    uint64_t node = (uint64_t) new rbt_node_t(4);
    r->insert_node(node);

    rbt_verify(r);

    shared_ptr<RBT_INT> ans = make_shared<RBT_INT>(
            "(5,(2,(1,#,#),(4,#,#)),(11,(7,#,(8,#,#)),(14,#,(15,#,#))))",
            "B"
                    "B"
                        "B##"
                        "B##"
                    "B"
                        "B#R##"
                        "B#R##");

    rbt_verify(ans);

    assert(rbt_compare(r, ans));

    // randomly insert values
    int loops = 50000;
    int iteration = 1000;

    for (int i = 0; i < loops; ++ i) {
        if (i % iteration == 0) {
            printf("insert %d / %d\n", i, loops);
        }

        uint64_t key = rand() % 1000000;
        node = (uint64_t) new rbt_node_t(key);
        r->insert_node(node);

//        std::printf("insert node %d\n", key);
        rbt_verify(r);
    }

    printf("\033[32;1m\tPass\033[0m\n");
}

static void test_delete() {
    printf("Testing Red-Black tree deletion ...\n");

    std::shared_ptr<RBT_INT> r;
    std::shared_ptr<RBT_INT> a;


    // no double black
    // bst case 2 - single child(right)
    r = make_shared<RBT_INT>("(10,"
                    "(5,#,(9,#,#)),"
                    "(15,#,#)"
                    ")",
                    "BB#R##B##");

    r->delete_node(r->rbt_find(5));

    a = make_shared<RBT_INT>("(10,"
                    "(9,#,#),"
                    "(15,#,#)"
                    ")",
                    "BB##B##");
    assert(rbt_compare(r, a));

    r = nullptr;
    a = nullptr;

    // no double black
    // bst case 2 - single child(left)
    r = make_shared<RBT_INT>("(10,"
                    "(5,(9,#,#),#),"
                    "(15,#,#)"
                    ")",
                    "BBR###B##");

    r->delete_node(r->rbt_find(5));

    a = make_shared<RBT_INT>("(10,"
                    "(9,#,#),"
                    "(15,#,#)"
                    ")",
                    "BB##B##");
    assert(rbt_compare(r, a));

    r = nullptr;
    a = nullptr;

    // no double black
    // bst case 3.1 - 2 child
    // x->right->left == NULL
    // (R, T2, (B, #, (R, #, #)))
    // tarns to bst case 2(one child)
    r = make_shared<RBT_INT>("(10,"
                             "(5,"           // delete - red
                             "(2,#,#),"      // T2
                             "(6,#,(7,#,#))" // successor
                             "),"
                             "(15,#,#)"      // T2
                             ")",
                             "BRB##B#R##B##");

    r->delete_node(r->rbt_find(5));

    a = make_shared<RBT_INT>("(10,"
                             "(6,"      // T2
                             "(2,#,#)," // T2
                             "(7,#,#)"
                             "),"
                             "(15,#,#)" // T2
                             ")",
                             "BRB##B##B##");
    assert(rbt_compare(r, a));

    r = nullptr;
    a = nullptr;

    // no double black
    // bst case 3.1 - 2 child
    // x->right->left == NULL
    // (B, T2, (B, #, (R, #, #)))
    // tarns to bst case 2(one child)
    r = make_shared<RBT_INT>("(10,"
                             "(5,"           // delete - black
                             "(2,#,#),"      // T2
                             "(6,#,(7,#,#))" // successor
                             "),"
                             "(15,(12,#,#),(16,#,#))" // T3
                             ")",
                             "BBB##B#R##BB##B##");

    r->delete_node(r->rbt_find(5));

    a = make_shared<RBT_INT>("(10,"
                             "(6,"      // T3
                             "(2,#,#)," // T2
                             "(7,#,#)"
                             "),"
                             "(15,(12,#,#),(16,#,#))" // T3
                             ")",
                             "BBB##B##BB##B##");
    assert(rbt_compare(r, a));

    r = nullptr;
    a = nullptr;

    // no double black
    // bst case 3.1 - 2 child
    // (B, T1, (R, #, #))
    // tarns to bst case 1(non child)
    r = make_shared<RBT_INT>("(10,"
                             "(5,"      // delete - black
                             "(2,#,#)," // T2
                             "(7,#,#)"  // successor
                             "),"
                             "(15,#,#)" // T2
                             ")",
                             "BBR##R##B##");

    r->delete_node(r->rbt_find(5));

    a = make_shared<RBT_INT>("(10,"
                             "(7,"
                             "(2,#,#),"
                             "#"
                             "),"
                             "(15,#,#)" // T2
                             ")",
                             "BBR###B##");
    assert(rbt_compare(r, a));

    r = nullptr;
    a = nullptr;

    // no double black
    // bst case 3.2 - 2 child
    // successor (R, #, #)
    // trans to bst case 1(non child)
    r = make_shared<RBT_INT>("(4,"
                             "(2,(1,#,#),(3,#,#)),"
                             "(6," // delete
                             "(5,#,#),"
                             "(10,"
                             "(8,(7,#,#),(9,#,#))," // 7 successor
                             "(11,#,#)"
                             "),"
                             ")"
                             ")",
                             "BBB##B##BB##RBR##R##B##");

    r->delete_node(r->rbt_find(6));

    a = make_shared<RBT_INT>("(4,"
                             "(2,(1,#,#),(3,#,#)),"
                             "(7,"  // successor
                             "(5,#,#),"
                             "(10,"
                             "(8,#,(9,#,#)),"
                             "(11,#,#)"
                             "),"
                             ")"
                             ")",
                             "BBB##B##BB##RB#R##B##");
    assert(rbt_compare(r, a));

    r = nullptr;
    a = nullptr;

    // no double black
    // bst case 3.2 - 2 child
    // successor (B, #, (R, #, #)), parent red
    // trans to bst case 2(one child)
    r = make_shared<RBT_INT>("(8,"
                             "(4,(2,(1,#,#),(3,#,#)),(6,(5,#,#),(7,#,#)))," // T3
                             "(12," // delete
                             "(10,(9,#,#),(11,#,#)),"
                             "(17,"
                             "(15,(13,#,(14,#,#)),(16,#,#))," // 13 successor
                             "(19,(18,#,#),(20,#,#))"
                             "),"
                             ")"
                             ")",
                             "BBBB##B##BB##B##BBB##B##BRB#R##B##RB##B##");

    r->delete_node(r->rbt_find(12));

    a = make_shared<RBT_INT>("(8,"
                             "(4,(2,(1,#,#),(3,#,#)),(6,(5,#,#),(7,#,#)))," // T4
                             "(13," // successor
                             "(10,(9,#,#),(11,#,#)),"
                             "(17,"
                             "(15,(14,#,#),(16,#,#))," // recoloring
                             "(19,(18,#,#),(20,#,#))"
                             "),"
                             ")"
                             ")",
                             "BBBB##B##BB##B##BBB##B##BRB##B##RB##B##");
    assert(rbt_compare(r, a));

    r = nullptr;
    a = nullptr;

    // no double black
    // bst case 3.2 - 2 child
    // successor (B, #, (R, #, #)), parent black
    // trans to bst case 2(one child)
    r = make_shared<RBT_INT>("(8,"
                             "(4,(2,(1,#,#),(3,#,#)),(6,(5,#,#),(7,#,#)))," // T4
                             "(12," // delete
                             "(10,(9,#,#),(11,#,#)),"
                             "(17,"
                             "(15,(13,#,(14,#,#)),(16,#,#))," // 13 successor
                             "(19,(18,#,#),(20,#,#))"
                             "),"
                             ")"
                             ")",
                             "BBBB##B##BB##B##BBB##B##RBB#R##B##BB##B##");

    r->delete_node(r->rbt_find(12));

    a = make_shared<RBT_INT>("(8,"
                             "(4,(2,(1,#,#),(3,#,#)),(6,(5,#,#),(7,#,#)))," // T4
                             "(13," // successor
                             "(10,(9,#,#),(11,#,#)),"
                             "(17,"
                             "(15,(14,#,#),(16,#,#))," // recoloring
                             "(19,(18,#,#),(20,#,#))"
                             "),"
                             ")"
                             ")",
                             "BBBB##B##BB##B##BBB##B##RBB##B##BB##B##");
    assert(rbt_compare(r, a));

    r = nullptr;
    a = nullptr;

    // delete red node
    r = make_shared<RBT_INT>("(10,"
                             "(5,(2,#,#),(9,#,#)),"
                             "(30,(25,#,#),(40,(38,#,#),#))"
                             ")",
                             "BRB##B##RB##BR###");

    r->delete_node(r->rbt_find(38));

    a = make_shared<RBT_INT>("(10,"
                             "(5,(2,#,#),(9,#,#)),"
                             "(30,(25,#,#),(40,#,#))"
                             ")",
                             "BRB##B##RB##B##");
    assert(rbt_compare(r, a));

    r = nullptr;
    a = nullptr;

    // delete black node - simple
    r = make_shared<RBT_INT>("(10,"
                             "(5,(2,#,#),(9,#,#)),"
                             "(30,(25,#,#),(40,(35,#,(38,#,#)),(50,#,#)))"
                             ")",
                             "BBB##B##BB##RB#R##B##");

    r->delete_node(r->rbt_find(30));

    a = make_shared<RBT_INT>("(10,"
                             "(5,(2,#,#),(9,#,#)),"
                             "(35,(25,#,#),(40,(38,#,#),(50,#,#)))"
                             ")",
                             "BBB##B##BB##RB##B##");
    assert(rbt_compare(r, a));

    r = nullptr;
    a = nullptr;

    // ====================================================== //
    // delete a double black node: case 0x7
    // double black = 15
    //  1. sibling(s) black = 30
    //  2. both sibling's child(nf) black = (NULL, NULL)
    // double black gives black to parent
    // parent red, then black
    // sibling red
    r = make_shared<RBT_INT>("(10,"
                             "(5,#,#),"
                             "(20,(15,#,#),(30,#,#))"
                             ")",
                             "BB##RB##B##");

    r->delete_node(r->rbt_find(15));

    a = make_shared<RBT_INT>("(10,"
                             "(5,#,#),"
                             "(20,#,(30,#,#))"
                             ")",
                             "BB##B#R##");
    assert(rbt_compare(r, a));

    r = nullptr;
    a = nullptr;

    // delete a double black node: case 0xF
    // double black = 15
    //  1. sibling(s) black = 30
    //  2. both sibling's child(nf) black = (NULL, NULL)
    // double black gives black to parent
    // parent black, then parent double black, continue to parent, until root
    // sibling red
    r = make_shared<RBT_INT>("(10,"
                             "(5,(1,#,#),(7,#,#)),"
                             "(20,(15,#,#),(30,#,#))"
                             ")",
                             "BBB##B##BB##B##");

    r->delete_node(r->rbt_find(15));

    a = make_shared<RBT_INT>("(10,"
                             "(5,(1,#,#),(7,#,#)),"
                             "(20,#,(30,#,#))"
                             ")",
                             "BRB##B##B#R##");
    assert(rbt_compare(r, a));

    r = nullptr;
    a = nullptr;

    // delete a double black node: case 0xB
    // double black = 15
    //  1. sibling(s) **RED** = 30
    //  2. both sibling's child(nf) black = (NULL, NULL)
    // double black gives black to parent then to sibling
    // parent black, sibling red ==> parent red, sibling black(has rotation -> trans to case 0x4-0x7)
    r = make_shared<RBT_INT>("(10,"
                             "(5,(1,#,#),(7,#,#)),"
                             "(20,(15,#,#),(30,(25,#,#),(40,#,#)))"
                             ")",
                             "BBB##B##BB##RB##B##");

    r->delete_node(r->rbt_find(15));

    a = make_shared<RBT_INT>("(10,"
                             "(5,(1,#,#),(7,#,#)),"
                             "(30,(20,#,(25,#,#)),(40,#,#))"
                             ")",
                             "BBB##B##BB#R##B##");
    assert(rbt_compare(r, a));

    r = nullptr;
    a = nullptr;

    // delete
    // sibling black, far child red, near child black —— case 0X6, 0XE
    // sibling black, far child black, near child red —— case 0x4,0x5,0xC,0xD
    r = make_shared<RBT_INT>("(10,"
                             "(5,(1,#,#),(7,#,#)),"
                             "(30,(25,(20,#,#),(28,#,#)),(40,#,#))"
                             ")",
                             "BBB##B##BRB##B##B##");

    r->delete_node(r->rbt_find(1));

    a = make_shared<RBT_INT>("(25,"
                             "(10,(5,#,(7,#,#)),(20,#,#)),"
                             "(30,(28,#,#),(40,#,#))"
                             ")",
                             "BBB#R##B##BB##B##");
    assert(rbt_compare(r, a));

    r = nullptr;
    a = nullptr;

    // A COMPLETE TEST CASE
    // sibling red
    r = make_shared<RBT_INT>("(50,"
                             "(20,(15,#,#),(35,#,#)),"
                             "(65,"
                             "(55,#,#),"
                             "(70,(68,#,#),(80,#,(90,#,#)))"
                             ")"
                             ")",
                             "BBB##B##BB##RB##B#R##");

    // delete 55 - sibling's 2 black
    r->delete_node(r->rbt_find(55));

    a = make_shared<RBT_INT>("(50,"
                             "(20,(15,#,#),(35,#,#)),"
                             "(70,"
                             "(65,#,(68,#,#)),"
                             "(80,#,(90,#,#))"
                             ")"
                             ")",
                             "BBB##B##BB#R##B#R##");
    assert(rbt_compare(r, a));
    a = nullptr;

    // delete 20 - root double black
    r->delete_node(r->rbt_find(20));

    a = make_shared<RBT_INT>("(50,"
                             "(35,(15,#,#),#),"
                             "(70,"
                             "(65,#,(68,#,#)),"
                             "(80,#,(90,#,#))"
                             ")"
                             ")",
                             "BBR###RB#R##B#R##");
    assert(rbt_compare(r, a));
    a = nullptr;

    // delete 90 - red node
    r->delete_node(r->rbt_find(90));

    a = make_shared<RBT_INT>("(50,"
                             "(35,(15,#,#),#),"
                             "(70,"
                             "(65,#,(68,#,#)),"
                             "(80,#,#)"
                             ")"
                             ")",
                             "BBR###RB#R##B##");
    assert(rbt_compare(r, a));
    a = nullptr;

    // delete 80 - sibling black, near child red, far child black
    r->delete_node(r->rbt_find(80));

    a = make_shared<RBT_INT>("(50,"
                             "(35,(15,#,#),#),"
                             "(68,"
                             "(65,#,#),"
                             "(70,#,#)"
                             ")"
                             ")",
                             "BBR###RB##B##");
    assert(rbt_compare(r, a));
    a = nullptr;

    // delete 50 - root, and having 1B 1R child, no parent nor sibling
    r->delete_node(r->rbt_find(50));

    a = make_shared<RBT_INT>("(65,"
                             "(35,(15,#,#),#),"
                             "(68,#,(70,#,#))"
                             ")"
                             ")",
                             "BBR###B#R##");
    assert(rbt_compare(r, a));
    a = nullptr;

    // delete 35 - having red child
    r->delete_node(r->rbt_find(35));

    a = make_shared<RBT_INT>("(65,"
                             "(15,#,#),"
                             "(68,#,(70,#,#))"
                             ")"
                             ")",
                             "BB##B#R##");
    assert(rbt_compare(r, a));
    a = nullptr;

    // delete 15 - far child red sibling black
    r->delete_node(r->rbt_find(15));

    a = make_shared<RBT_INT>("(68,(65,#,#),(70,#,#))", "BB##B##");
    assert(rbt_compare(r, a));
    a = nullptr;

    // delete 65 - both s-child black
    r->delete_node(r->rbt_find(65));

    a = make_shared<RBT_INT>("(68,#,(70,#,#))", "B#R##");
    assert(rbt_compare(r, a));
    a = nullptr;

    printf("\033[32;1m\tPass\033[0m\n");
}

static void test_insert_delete() {
    printf("Testing Red-Black Tree insertion and deletion ...\n");

    std::shared_ptr<RBT_INT> tree = make_shared<RBT_INT>(NULL_NODE);

    // insert
    int loops = 50000;
    int iteration = 1000;

    vector<uint64_t> array(loops);
    for (int i = 0; i < loops; ++i) {
        if (i % iteration == 0) {
            printf("insert %d / %d\n", i, loops);
        }

        uint64_t key = rand() % 1000000;
//        ::printf("insert node %lld", key);
        uint64_t node = (uint64_t)new RBT_INT_NODE(key);

        tree->insert_node(node);
        rbt_verify(tree);

        array[i] = key;
    }

    // mark
    for (int i = 0; i < loops; ++i) {
        if (i % iteration == 0) {
            printf("delete %d / %d\n", i, loops);
        }

        int index = rand() % loops;

        tree->delete_node(tree->rbt_find(array[index]));
        rbt_verify(tree);

        array[index] = 0xFFFFFFFFFFFFFFFF;
    }

    // sweep
    for (int i = 0; i < loops; ++i) {
        if (array[i] != 0xFFFFFFFFFFFFFFFF) {
            tree->delete_node(tree->rbt_find(array[i]));
            rbt_verify(tree);
        }
    }

    assert(tree->get_root() == NULL_NODE);

    printf("\033[32;1m\tPass\033[0m\n");
}

int main() {
//    RBT_INT *test = new RBT_INT("(10,(5,#,(9,#,#)),(15,#,#))",
//                                "BB#R##B##");
//    rbt_verify(test);

//    test_rotation();
//    test_insert();
//    test_delete();
    test_insert_delete();
    return 0;
}