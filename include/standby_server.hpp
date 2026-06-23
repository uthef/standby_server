#ifndef STANDBY_SERVER_HPP
#define STANDBY_SERVER_HPP

#include <http_server.hpp>
#include <udp_socket.hpp>
#include <array>

namespace uthef {

class standby_server : public http_server {
private:
	udp_socket udp_client;
	std::array<char, 102> magic_packet;
	std::string remote_mac;
public:
	std::string redirect_url;
	std::string remote_ip;
	std::string base_url;
	in_port_t remote_udp_port = 9;

	standby_server(const char *file_name, unsigned int max_clients = 128);
	udp_socket *get_udp_client();
	bool set_remote_mac(const char *value);

protected:
	void handle_request(http_server::request &request_data, 
			http_server::response &response_data) override;
private:
	bool send_magic_packet();
	static bool compare_url(std::string &url, const char *endpoint);
}; 

}

#endif
