#ifndef ENDPOINT_HPP
#define ENDPOINT_HPP

#include <string>
#include <array>
#include <http_server.hpp>
#include <standby_server.hpp>
#include <udp_socket.hpp>

namespace uthef {

class standby_server;

enum endpoint_action {
	WAKE_REDIR = 0,
	RUN_COMMAND = 1
};

enum endpoint_method {
	GET = 0,
	POST = 1
};

class endpoint {
private:
	std::string url;
	endpoint_action action = WAKE_REDIR;
	endpoint_method method = GET;
	bool authentication = false;
protected:
	endpoint(std::string url, endpoint_action, endpoint_method, bool authentication);
public:
	std::string get_url() const;
	endpoint_action get_action() const;
	endpoint_method get_method() const;
	bool is_authentication_required() const;

	virtual void handle_request(standby_server &server, http_server::request &, http_server::response &);

	static const char *map_method(endpoint_method);
};

class wake_redir_endpoint : public endpoint {
private:
	std::string ip;
	std::string mac;
	std::string redirect_url;
	std::array<char, 102> packet;

	in_port_t remote_udp_port = 9;

	bool send_magic_packet(udp_socket &udp_client);
public:
	wake_redir_endpoint(std::string url, endpoint_method, bool authentication);

	void set_ip(std::string ip);
	bool set_mac(std::string mac);
	void set_redirect_url(std::string url);

	std::string get_ip() const;
	std::string get_mac() const;
	std::string get_redirect_url() const;

	const char *get_packet();

	void handle_request(standby_server &server, http_server::request &, http_server::response &) override;
};

class run_command_endpoint : public endpoint {
private:
	std::string command;
public:
	run_command_endpoint(std::string url, endpoint_method, bool authentication);
	
	void set_command(std::string command);
	std::string get_command() const;

	void handle_request(standby_server &server, http_server::request &, http_server::response &) override;
};

}

#endif
