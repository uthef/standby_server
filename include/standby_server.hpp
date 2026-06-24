#ifndef STANDBY_SERVER_HPP
#define STANDBY_SERVER_HPP

#include <http_server.hpp>
#include <udp_socket.hpp>
#include <array>
#include <vector>
#include <endpoint.hpp>
#include <settings.hpp>
#include <memory>

namespace uthef {

class endpoint;
class run_command_endpoint;
class wake_redir_endpoint;

class standby_server : public http_server {
private:
	udp_socket udp_client;
	std::string auth_header;
	std::vector<std::unique_ptr<endpoint>> endpoints;
public:
	std::string base_url;

	standby_server(const char *file_name, unsigned int max_clients = 128);
	udp_socket *get_udp_client();

	void set_credentials(std::string user, std::string password) ;
	void add_endpoint(endpoint *);
	void parse_endpoints(settings &);
	void clear_endpoints();

protected:
	void handle_request(http_server::request &request_data, 
			http_server::response &response_data) override;
private:
	static bool compare_url(std::string &url, const char *endpoint);
}; 

}

#endif
