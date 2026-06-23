#include <magic_datagram.hpp>
#include <string>
#include <stdint.h>
#include <memory.h>

bool uthef::build_magic_datagram(const char *mac_addr, std::array<char, 102> &buffer) {
	char mac_addr_bytes[6];
	int mac_addr_bytes_pos = 0;
	bool separator_expected = false;
	std::string hexnum;

	const char *iter = mac_addr;

	while (*iter) {
		char c = *iter;

		if (c == ':') {
			if (separator_expected && mac_addr_bytes_pos < sizeof(mac_addr_bytes)) {
				separator_expected = false;
				iter++;
				continue;
			}

			return false;
		}

		if (mac_addr_bytes_pos >= sizeof(mac_addr_bytes)) {
			return false;
		}
		
		if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
			hexnum.push_back(c);
		else
			return false;

		separator_expected = false;

		if (hexnum.size() >= 2) {
			try {
				char b = std::stoi(hexnum, 0, 16);
				mac_addr_bytes[mac_addr_bytes_pos++] = b;
				separator_expected = true;
			}
			catch (const std::exception &e) {
				return false;
			}

			hexnum.clear();
		}

		iter++;
	}

	if (mac_addr_bytes_pos != sizeof(mac_addr_bytes)) {
		return false;
	}

	memset(buffer.data(), 0xff, buffer.size());

	for (int i = 1; i < buffer.size() / 6; i++) {
		memcpy(buffer.data() + 6 * i, mac_addr_bytes, sizeof(mac_addr_bytes));
	}

	return true;
}
