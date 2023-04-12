#include <cstdio>
#include <cassert>

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

void rbt_verify(const RBT_INT *rbt) {
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

bool rbt_compare(const RBT_INT *lhs, const RBT_INT *rhs) {
    if (lhs == nullptr && rhs == nullptr) {
        return true;
    }

    if (lhs == nullptr || rhs == nullptr) {
        // 由于两者全部为空时走上面条件判断，因此此时对应的是两者有一个为空
        return false;
    }

    // 仅仅作为接口传入进去，用于访问RBT中的相关函数
    RBT_INT *interface = new RBT_INT(NULL_NODE);
    bool res = rbt_compare(lhs->get_root(), rhs->get_root(), interface);
    delete interface;

    return res;
}

static void test_insert() {
    printf("Testing Red-Black tree insertion ...\n");
    RBT_INT *r = new RBT_INT(
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

    RBT_INT *ans = new RBT_INT(
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

    delete ans;

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

    delete r;
    printf("\033[32;1m\tPass\033[0m\n");
}

static void test_delete() {
    printf("Testing Red-Black tree deletion ...\n");

    RBT_INT *r = nullptr;
    RBT_INT *a = nullptr;

    // no double black
    // bst case 2 - single child
}

int main() {
//    RBT_INT *test = new RBT_INT("(10,(5,#,(9,#,#)),(15,#,#))",
//                                "BB#R##B##");
//    rbt_verify(test);

    test_insert();
    return 0;
}