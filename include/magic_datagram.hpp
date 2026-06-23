#ifndef MAGIC_DATAGRAM_HPP
#define MAGIC_DATAGRAM_HPP

#include <array>

namespace uthef {

bool build_magic_datagram(const char *mac_addr, std::array<char, 102> &buffer);

}

#endif
