#include <endpoint.hpp>
#include <magic_datagram.hpp>
#include <iostream>
#include <run_command.hpp>
#include <iomanip>

using namespace uthef;

endpoint::endpoint(std::string url, endpoint_action action, endpoint_method method, bool authentication) 
		: url(url), action(action), method(method), authentication(authentication) {

}

std::string endpoint::get_url() const {
	return url;
}

endpoint_action endpoint::get_action() const {
	return action;
}

endpoint_method endpoint::get_method() const {
	return method;
}

bool endpoint::is_authentication_required() const {
	return authentication;
}

void endpoint::handle_request(standby_server &server, 
		http_server::request &request_data, http_server::response &response_data) {
	response_data.code = 200;
}

const char *endpoint::map_method(endpoint_method method) {
	switch (method) {
		case GET:
			return "GET";
		case POST:
			return "POST";
	}

	return nullptr;
}

wake_redir_endpoint::wake_redir_endpoint(std::string url, endpoint_method method, bool authentication) 
		: endpoint(url, WAKE_REDIR, method, authentication) {
		
}

void wake_redir_endpoint::set_ip(std::string ip) {
	this->ip = ip;
}

bool wake_redir_endpoint::set_mac(std::string mac) {
	this->mac = mac;
	return build_magic_datagram(mac.c_str(), packet);
}

void wake_redir_endpoint::set_redirect_url(std::string url) {
	this->redirect_url = url;
}

std::string wake_redir_endpoint::get_ip() const {
	return ip;
}

std::string wake_redir_endpoint::get_mac() const {
	return mac;
}

std::string wake_redir_endpoint::get_redirect_url() const {
	return redirect_url;
}

const char *wake_redir_endpoint::get_packet() {
	return packet.data();
}

void wake_redir_endpoint::handle_request(standby_server &server, 
		http_server::request &request_data, http_server::response &response_data) {

	if (!ip.empty() && !mac.empty()) {
		if (!send_magic_packet(*server.get_udp_client())) {
			response_data.code = 500;
			return;
		}
		
		if (redirect_url.empty()) {
			std::stringstream ss;
			ss << "destination=" << ip << ":" << remote_udp_port << std::endl;
			ss << "remote_mac=";
			
			for (int i = 0; i < 6; i++) {
				if (i > 0)
					ss << ":";

				ss << std::hex << std::setw(2) << std::setfill('0') << std::uppercase <<  
					((int)(unsigned char) *(packet.data() + 6 + i));
			}

			ss << std::endl;
			
			response_data.code = 200;
			response_data.body = ss.str();

			return;
		}
	}

	if (!redirect_url.empty()) {
		response_data.headers["Location"] = redirect_url;
		response_data.code = 302;
		return;
	}

	response_data.code = 500;
}

bool wake_redir_endpoint::send_magic_packet(udp_socket &udp_client) {
	if (mac.empty())
		return false;

	int result = udp_client.send(packet.data(), packet.size(), ip.c_str(), remote_udp_port);

	if (result <= 0) {
		std::cerr << "Failed to send datagram" << std::endl;
		return false;
	}

	return true;
}

run_command_endpoint::run_command_endpoint(std::string url, endpoint_method method, bool authentication)
		: endpoint(url, RUN_COMMAND, method, authentication) {
}

void run_command_endpoint::set_command(std::string command) {
	this->command = command;
}

std::string run_command_endpoint::get_command() const {
	return this->command;
}

void run_command_endpoint::handle_request(standby_server &server, 
		http_server::request &request_data, http_server::response &response_data) {
		
	std::string form;

	if (request_data.method == "POST") {
		form = request_data.body;
	}

	std::string full_command;
	bool eq_sign = false;

	for (int i = 0; i < form.size(); i++) {
		char c = form.at(i);

		if (c == '=') {
			if (full_command.empty() || full_command.back() == '&') {
				response_data.code = 400;
				return;
			}
			eq_sign = true;
		}
		else if (c == '&') {
			if (!eq_sign) {
				response_data.code = 400;
				return;
			}

			full_command.push_back(' ');
			eq_sign = false;
			continue;
		}
		else if (!isalnum(c) && c != '_') {
			response_data.code = 400;
			return;
		}

		full_command.push_back(c);
	}

	if ((!form.empty() && !eq_sign) || full_command.back() == '=') {
		response_data.code = 400;
		return;
	}
	
	if (!full_command.empty()) {
		full_command.push_back(' ');
	}

	full_command.append(command);

	if (command.empty() || !run_command(full_command, response_data.body)) {
		response_data.body.clear();
		response_data.code = 500;
		return;
	}

	response_data.code = 200;
}
