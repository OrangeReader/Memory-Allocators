#ifndef MYMALLOC_SMALL_LIST_H
#define MYMALLOC_SMALL_LIST_H

#include "linked-list.h"

// ================================================ //
//    The implementation of the small linked list   //
// ================================================ //
class SMALL_FREE_LINKED_LIST final : public LINKED_LIST {
public:
    SMALL_FREE_LINKED_LIST(uint64_t head, uint64_t count)
        : head_(head), count_(count) {}

    // 由于heap上的空闲块，如果全部释放掉那么就变成了隐式空闲链表
    // ⭐ small list和其他空闲块不同，它利用了head 和 foot 设置了 prev 和 next
    // 因此析构时需要将其恢复成前一个隐式空闲链表结构
    ~SMALL_FREE_LINKED_LIST() override {
        delete_list();
    }

protected:
    uint64_t get_head() const override;
    bool set_head(uint64_t new_head) override;

    uint64_t get_count() const override;
    bool set_count(uint64_t new_count) override;

    bool destruct_node(uint64_t node) override;

    bool is_nodes_equal(uint64_t first, uint64_t second) override;

    uint64_t get_node_prev(uint64_t node) override;
    bool set_node_prev(uint64_t node, uint64_t prev) override;

    uint64_t get_node_next(uint64_t node) override;
    bool set_node_next(uint64_t node, uint64_t next) override;

private:
    void delete_list();

    uint64_t head_ = 0;
    uint64_t count_ = 0;
};

// interface
void small_list_init();
void small_list_insert(uint64_t free_header);
void small_list_delete(uint64_t free_header);
extern std::shared_ptr<SMALL_FREE_LINKED_LIST> small_list;
void check_size_list_correctness(const std::shared_ptr<LINKED_LIST> &list, uint32_t min_size, uint32_t max_size);
void small_list_check_free_blocks();

#endif //MYMALLOC_SMALL_LIST_H
