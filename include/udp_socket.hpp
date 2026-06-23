#ifndef UDP_SOCKET_HPP
#define UDP_SOCKET_HPP

#include <netinet/in.h>
#include <string>

namespace uthef {

class udp_socket {
private:
	std::string host;
	int sockd = 0;
	bool bound = false;
	struct sockaddr_in addr;
public:
	udp_socket(const char *host, unsigned short port = 0);
	udp_socket(const udp_socket &) = delete;
	udp_socket(udp_socket &&);
	~udp_socket();

	udp_socket &operator =(const udp_socket &) = delete;
	udp_socket &operator =(udp_socket &&);
	
	bool is_bound() const;
	void bind(bool broadcast = true);
	void close();
	in_port_t real_port();
	int send(const char *buffer, size_t buffer_size, const char *ip, unsigned short port);
private:
	udp_socket &move(udp_socket &);
};

}

#endif
