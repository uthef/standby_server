#include <signal.h>
#include <standby_server.hpp>
#include <settings.hpp>
#include <iostream>
#include <filesystem>

void handle_interruption(int);

uthef::standby_server* st_server = nullptr;

int main(const int argc, const char **argv) {
	struct sigaction sigint_action;
	sigemptyset(&sigint_action.sa_mask);
	sigint_action.sa_flags = 0;
	sigint_action.sa_handler = handle_interruption;

	sigaction(SIGINT, &sigint_action, nullptr);

	std::string config_path = argc > 1 ? argv[1] : "standby_server.ini";
	uthef::settings config(config_path);

	if (std::filesystem::exists(config_path)) {
		config.load();
	}
	else if (argc > 1) {
		std::cerr << "Configuration file " << "\"" << config_path << "\"" 
			<< "is not found" << std::endl;
		exit(-1);		
	}

	std::string tcp_socket = config.get("tcp_socket");

	if (tcp_socket.empty())
		tcp_socket = "standby_server.socket";

	uthef::standby_server server(tcp_socket.c_str());
	st_server = &server;
	server.base_url = config.get("base_url");
	server.remote_ip = config.get("remote_ip");
	server.redirect_url = config.get("redirect_url");

	if (!config.get("remote_mac").empty()) {
		if (!server.set_remote_mac(config.get("remote_mac").c_str())) {
			std::cerr << "Invalid MAC address" << std::endl;
			exit(-1);
		}
	}

	server.listen();
	
	if (!server.is_listening()) {
		std::cerr << "Could not open TCP socket" << std::endl;
		server.get_udp_client()->close();
		exit(-1);
	}

	server.poll();
	return 0;
}

void handle_interruption(int signal) {
	if (signal != SIGINT)
		return;
	
	puts("\nINTERRUPTED");

	if (st_server) {
		st_server->get_udp_client()->close();
		st_server->close();
	}

	exit(0);
}
