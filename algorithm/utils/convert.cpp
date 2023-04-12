#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cassert>

#include "utils.h"

// covert string to int64_t
uint64_t string2uint(const char *str) {
    return string2uint_range(str, 0, -1);
}

// 能够处理十六进制的字符串
string2uint_state_t string2uint_next(string2uint_state_t state, char c, uint64_t &bmap) {
    // state - parsing state. see the common.h marcos
    // bmap - the bitmap of the value
    // return value - next state
    switch (state) {
        case STRING2UINT_LEADING_SPACE:
            if (c == '0') {
                // 1. positive dec value with leading zeros
                // 2. hex number (positive only)
                bmap = 0;
                return STRING2UINT_FIRST_ZERO;
            } else if ('1' <= c && c <= '9') {
                // positive dec
                bmap = c - '0';
                return STRING2UINT_POSITIVE_DEC;
            } else if (c == '-') {
                // signed negative value
                return STRING2UINT_NEGATIVE;
            } else if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                // skip leading spaces
                return STRING2UINT_LEADING_SPACE;
            }
            // fail
            return STRING2UINT_FAILED;
        case STRING2UINT_FIRST_ZERO:
            // check zero
            if ('0' <= c && c <= '9') {
                bmap = c - '0';
                return STRING2UINT_POSITIVE_DEC;
            } else if (c == 'x' || c == 'X') {
                return STRING2UINT_POSITIVE_HEX;
            } else if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                // zero only
                assert(bmap == 0);
                return STRING2UINT_ENDING_SPACE;
            }
            // fail
            return STRING2UINT_FAILED;
        case STRING2UINT_POSITIVE_DEC:
            // dec number
            // signed or unsigned
            if ('0' <= c && c <= '9') {
                // positive value
                // bmap = bmap * 2 + bmap * 8 + c - '0'
                //      = bmap * 10 + c - '0'
                bmap = (bmap << 1) + (bmap << 3) + c - '0';
                return STRING2UINT_POSITIVE_DEC;
            } else if (c == ' ') {
                return STRING2UINT_ENDING_SPACE;
            }
            // fail
            return STRING2UINT_FAILED;
        case STRING2UINT_POSITIVE_HEX:
            // hex number
            if ('0' <= c && c <= '9') {
                bmap = (bmap << 4) + c - '0';
                return STRING2UINT_POSITIVE_HEX;
            } else if ('a' <= c && c <= 'f') {
                bmap = (bmap << 4) + c - 'a' + 10;
                return STRING2UINT_POSITIVE_HEX;
            } else if ('A' <= c && c <= 'F') {
                bmap = (bmap << 4) + c - 'A' + 10;
                return STRING2UINT_POSITIVE_HEX;
            } else if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                return STRING2UINT_ENDING_SPACE;
            }
            // fail
            return STRING2UINT_FAILED;
        case STRING2UINT_NEGATIVE:
            // negative
            // set the bit map of the negative value till now
            if ('1' <= c && c <= '9') {
                bmap = ~(c - '0') + 1;
                return STRING2UINT_NEGATIVE_DEC;
            } else if (c == '0') {
                bmap = 0;
                return STRING2UINT_NEGATIVE_FIRST_ZERO;
            }
            // fail
            return STRING2UINT_FAILED;
        case STRING2UINT_NEGATIVE_FIRST_ZERO:
            // check zero
            if ('0' <= c && c <= '9') {
                bmap = ~(c - '0') + 1;
                return STRING2UINT_NEGATIVE_DEC;
            } else if (c == 'x' || c == 'X') {
                return STRING2UINT_NEGATIVE_HEX;
            } else if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                // zero only
                assert(bmap == 0);
                return STRING2UINT_ENDING_SPACE;
            }
            // fail
            return STRING2UINT_FAILED;
        case STRING2UINT_NEGATIVE_DEC:
            // dec number
            if ('0' <= c && c <= '9') {
                bmap = (bmap << 1) + (bmap << 3) + c - '0';
                return STRING2UINT_NEGATIVE_DEC;
            } else if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                return STRING2UINT_ENDING_SPACE;
            }
            // fail
            return STRING2UINT_FAILED;
        case STRING2UINT_NEGATIVE_HEX:
            // hex number
            if ('0' <= c && c <= '9') {
                bmap = (bmap << 4) + ~(c - '0') + 1;
                return STRING2UINT_NEGATIVE_HEX;
            } else if ('a' <= c && c <= 'f') {
                bmap = (bmap << 4) + ~(c - 'a' + 10) + 1;
                return STRING2UINT_NEGATIVE_HEX;
            } else if ('A' <= c && c <= 'F') {
                bmap = (bmap << 4) + ~(c - 'A' + 10) + 1;
                return STRING2UINT_NEGATIVE_HEX;
            } else if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                return STRING2UINT_ENDING_SPACE;
            }
            // fail
            return STRING2UINT_FAILED;
        case STRING2UINT_ENDING_SPACE:
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                // skip tailing spaces
                return STRING2UINT_ENDING_SPACE;
            }
            // fail
            return STRING2UINT_FAILED;
        default:
            // fail
            return STRING2UINT_FAILED;
    }
}

uint64_t string2uint_range(const char *str, int start, int end) {
    // start: starting index inclusive
    // end: ending index inclusive
    // [start, end], when end = -1, like python, is the end of the string
    end = (end == -1) ? strlen(str) - 1 : end;

    uint64_t uv = 0;

    // DFA: deterministic finite automata to scan string and get value
    string2uint_state_t state = STRING2UINT_LEADING_SPACE;

    for (int i = start; i <= end; ++i) {
        char c = str[i];
        state = string2uint_next(state, c, uv);

        switch (state) {
            case STRING2UINT_FAILED:
                printf("string2uint: failed to transfer: %s\n", str);
                exit(0);
            default:
                break;
        }
    }
    return uv;
}
