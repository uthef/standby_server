#include <udp_socket.hpp>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <utility>
#include <memory.h>

using namespace uthef;

udp_socket::udp_socket(const char *host, unsigned short port) {
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if (host) {
		this->host = std::string(host);
	}
	else {
		this->host = "0.0.0.0";
	}
}

udp_socket::udp_socket(udp_socket &&other) {
	move(other);	
}

udp_socket::~udp_socket() {
	close();
}

udp_socket &udp_socket::operator =(udp_socket &&other) {
	return move(other);	
}

bool udp_socket::is_bound() const {
	return bound;
}

void udp_socket::bind(bool broadcast) {
	if (bound || host.empty())
		return;

	sockd = socket(AF_INET, SOCK_DGRAM, 0);

	if (sockd == -1)
		return;

	if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
		::close(sockd);
		return;
	}

	int result = ::bind(sockd, (sockaddr *)&addr, sizeof(addr));

	if (result == -1) {
		::close(sockd);
		return;
	}

	if (broadcast) {
		int opt = 1;
		::setsockopt(sockd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
	}

	bound = true;

	printf("UDP socket \"%s:%d\" is open\n", host.c_str(), real_port());
}

void udp_socket::close() {
	if (!bound)
		return;

	in_port_t port = real_port();
	
	::close(sockd);
	bound = false;

	printf("UDP socket \"%s:%d\" is closed\n", host.c_str(), port);
}

in_port_t udp_socket::real_port() {
	if (!bound)
		return 0;

	sockaddr real_addr;
	socklen_t real_addr_size = sizeof(addr);

	if (getsockname(sockd, &real_addr, &real_addr_size) < 0) {
		return 0;
	}

	if (real_addr_size != sizeof(sockaddr_in))
		return 0;

	return ntohs(((sockaddr_in *) &real_addr)->sin_port);
}

int udp_socket::send(const char *buffer, size_t buffer_size, const char *ip, unsigned short port) {
	if (!bound)
		return -1;
	
	struct sockaddr_in dest_addr;
	memset(&dest_addr, 0, sizeof(dest_addr));

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(port);

	if (!ip)
		return -1;

	if (inet_pton(AF_INET, ip, &dest_addr.sin_addr) <= 0) {
		return -1;
	}
		
	return sendto(sockd, buffer, buffer_size, 0, (sockaddr *) &dest_addr, sizeof(dest_addr));
}

udp_socket &udp_socket::move(udp_socket &other) {
	close();

	sockd = std::move(other.sockd);
	bound = std::move(other.bound);
	addr = std::move(other.addr);
	host = std::move(other.host);

	other.bound = false;
	
	return *this;
}
