#include <base64.hpp>

#include "stdbool.h"
#include "stdlib.h"
#include <stdio.h>

char sextet_to_char(unsigned char idx) {
    if (idx <= 25) return 'A' + idx;
    if (idx <= 51) return 'a' + (idx - 26);
    if (idx <= 61) return '0' + (idx - 52);
    if (idx == 62) return '+';
    if (idx == 63) return '/';

    return -1;
}

unsigned char char_to_sextet(char c, bool* validChar) {
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    if (c == '=') return 0;

    *validChar &= false;

    return 0;
}

char* uthef::alloc_base64_str(const char* src, const unsigned long src_size, unsigned long* out_size) {
    if (!src_size || !src) return 0;
    
    const char* iter = src;
    unsigned long b64_size = 0, idx = 0;

    {
        const unsigned long rem  = src_size % 3;
        b64_size = src_size + rem + src_size / 3;
        if (rem == 1) b64_size += 2;   
    }

    char* b64 = (char*)malloc(b64_size + 1);
    if (!b64) return 0;
    char* b64_iter = b64;

    while (idx < src_size) {
        unsigned char byte = *iter++;
        unsigned char sextet = byte >> 2;
        unsigned char next = byte << 4 & 0x3f;

        *b64_iter++ = sextet_to_char(sextet);
        idx++;

        if (idx++ >= src_size) {
            *b64_iter++ = sextet_to_char(next);
            *b64_iter++ = '=';
            *b64_iter++ = '=';
            break;
        }

        byte = *iter++;
        sextet = next | byte >> 4;
        next = (byte & 0x0f) << 2;

        *b64_iter++ = sextet_to_char(sextet);

        if (idx++ >= src_size) {
            *b64_iter++ = sextet_to_char(next);
            *b64_iter++ = '=';
            break;
        }

        byte = *iter++;
        sextet = next | byte >> 6; 
        *b64_iter++ = sextet_to_char(sextet);
        sextet = byte & 0x3f;
        *b64_iter++ = sextet_to_char(sextet);
    }

    *b64_iter = 0;
    if (out_size) *out_size = b64_size;

    return b64;
}   

char* uthef::alloc_base64_buffer(const char* src, unsigned long src_size, unsigned long* out_size) {
    if (!src || !src_size || !out_size) return 0;

    const char* src_iter = src;

    char* buff = (char*)malloc(src_size);
    if (!buff) return 0;
    char* buff_iter = buff;
    char c = 0;
    bool validChar = true;

    *out_size = 0;
    unsigned long count = 0;

    while ((c = *src_iter++)) {
        unsigned char sextet = char_to_sextet(c, &validChar);

        *buff_iter = sextet << 2;

        c = *src_iter++;
        if (c != '=') count++;

        if (!c) break;

        sextet = char_to_sextet(c, &validChar);

        *buff_iter++ |= sextet >> 4;
        *buff_iter = sextet << 4;

        c = *src_iter++;
        if (c != '=') count++;
        if (!c) break;

        sextet = char_to_sextet(c, &validChar);

        *buff_iter++ |= sextet >> 2;
        *buff_iter = sextet << 6;

        c = *src_iter++;
        if (c != '=') count++;

        if (!c) break;

        sextet = char_to_sextet(c, &validChar);

        if (!validChar) {
            free(buff);
            return 0;
        }

        *buff_iter++ |= sextet;
    }

    if (!validChar) {
        free(buff);
        return 0;
    }

    *out_size = count;

    return buff;
}
