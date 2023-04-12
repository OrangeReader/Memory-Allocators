#ifndef MYMALLOC_UTILS_H
#define MYMALLOC_UTILS_H

#include <cstdint>

// convert string dec or hex to the integer bitmap
using string2uint_state_t = enum {
    STRING2UINT_LEADING_SPACE,          // 前导部分
    STRING2UINT_FIRST_ZERO,             // 正数 + 第一位是0
    STRING2UINT_POSITIVE_DEC,           // 正数 + 十进制
    STRING2UINT_POSITIVE_HEX,           // 正数 + 十六进制
    STRING2UINT_NEGATIVE,               // 负数
    STRING2UINT_NEGATIVE_FIRST_ZERO,    // 负数 + 第一位是0
    STRING2UINT_NEGATIVE_DEC,           // 负数 + 十进制
    STRING2UINT_NEGATIVE_HEX,           // 负数 + 十六进制
    STRING2UINT_ENDING_SPACE,           // 结尾部分
    STRING2UINT_FAILED,                 // 转换失败
};

uint64_t string2uint(const char *str);
string2uint_state_t string2uint_next(string2uint_state_t state, char c, uint64_t *bmap);
uint64_t string2uint_range(const char *str, int start, int end);

#endif //MYMALLOC_UTILS_H
