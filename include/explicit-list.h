#ifndef MYMALLOC_EXPLICIT_LIST_H
#define MYMALLOC_EXPLICIT_LIST_H

#include "linked-list.h"

// ================================================ //
//   The implementation of the default linked list  //
// ================================================ //
// 我们采用数组模拟heap，数组的index就是virtual address，从而直接定位到相应的块
class EXPLICIT_FREE_LINKED_LIST final : public LINKED_LIST  {
public:
    // construct function
    EXPLICIT_FREE_LINKED_LIST(uint64_t head, uint64_t count)
            : LINKED_LIST(head, count) {}

    // 派生类中没有自己的成员变量需要被管理，因此设置为default即可
    // 然后调用父类的析构函数释放相关内存
    ~EXPLICIT_FREE_LINKED_LIST() final = default;

protected:
    // EXPLICIT_FREE_LINKED_LIST中需要override的函数
    bool destruct_node(uint64_t node) override;

    bool is_nodes_equal(uint64_t first, uint64_t second) override;

    uint64_t get_node_prev(uint64_t header_vaddr) override;
    bool set_node_prev(uint64_t header_vaddr, uint64_t prev_vaddr) override;

    uint64_t get_node_next(uint64_t header_vaddr) override;
    bool set_node_next(uint64_t header_vaddr, uint64_t next_vaddr) override;
};

#endif //MYMALLOC_EXPLICIT_LIST_H