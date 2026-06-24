#ifndef BASE64_HPP
#define BASE64_HPP

namespace uthef {

char* alloc_base64_str(const char* src, const unsigned long src_size, unsigned long* out_size);
char* alloc_base64_buffer(const char* src, unsigned long src_size, unsigned long* out_size);

}

#endif
