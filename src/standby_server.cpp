#include <standby_server.hpp>
#include <iostream>
#include <magic_datagram.hpp>
#include <cstring>
#include <iomanip>

using namespace uthef;

standby_server::standby_server(const char *file_name, unsigned int max_clients) 
		: http_server(file_name, max_clients), udp_client(NULL, 0) {
	udp_client.bind(true);

	if (!udp_client.is_bound()) {
		std::cerr << "Could not open UDP socket" << std::endl;
		exit(-2);
	}

	memset(magic_packet.data(), 0, magic_packet.size());
}

udp_socket *standby_server::get_udp_client() {
	return &udp_client;
}

bool standby_server::set_remote_mac(const char *value) {
	if (!value)
		return false;

	bool result = build_magic_datagram(value, magic_packet);

	if (result)
		remote_mac = value;

	return result;
}

void standby_server::handle_request(http_server::request &request_data, http_server::response &response_data) {
	response_data.code = 404;	

	auto base_url_path = "/" + base_url;

	if (strncmp(request_data.url.c_str(), base_url_path.c_str(), base_url_path.size()) != 0) {
		return;
	}
	
	std::string clean_url = request_data.url;

	if (base_url_path != "/")
		clean_url = request_data.url.substr(base_url_path.size());

	if (compare_url(clean_url, "/")) {
		if (request_data.method != "GET") {
			response_data.code = 405;
			return;
		}

		if (!send_magic_packet()) {
			response_data.code = 500;
			return;
		}

		if (!redirect_url.empty()) {
			response_data.headers["Location"] = redirect_url;
			response_data.code = 302;
			return;
		}

		response_data.code = 200;
		return;
	}

	if (compare_url(clean_url, "/wake")) {
		if (request_data.method != "GET") {
			response_data.code = 405;
			return;
		}

		response_data.code = 200;

		if (!send_magic_packet()) {
			response_data.code = 500;
			return;
		}

		std::stringstream ss;
		ss << "destination=" << remote_ip << ":" << remote_udp_port << std::endl;
		ss << "remote_mac=";
		
		for (int i = 0; i < 6; i++) {
			if (i > 0)
				ss << ":";

			ss << std::hex << std::setw(2) << std::setfill('0') << std::uppercase <<  
				((int)(unsigned char) *(magic_packet.data() + 6 + i));
		}

		ss << std::endl;
		
		response_data.body = ss.str();

		return;
	}
}

bool standby_server::send_magic_packet() {
	if (remote_mac.empty())
		return false;

	int result = udp_client.send(magic_packet.data(), magic_packet.size(), remote_ip.c_str(), remote_udp_port);

	if (result <= 0) {
		std::cerr << "Failed to send datagram" << std::endl;
		return false;
	}

	return true;
}

bool standby_server::compare_url(std::string &url, const char *endpoint) {
	bool result = strcasecmp(url.c_str(), endpoint) == 0;

	if (!result && strcmp(endpoint, "/") != 0 && 
			strcasecmp(url.c_str(), (std::string(endpoint) + "/").c_str()) == 0) {
		result = true;
	}

	return result;
}
