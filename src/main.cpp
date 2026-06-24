#include <signal.h>
#include <standby_server.hpp>
#include <settings.hpp>
#include <iostream>
#include <endpoint.hpp>
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

	server.parse_endpoints(config);

	if (!config.get("user").empty() && !config.get("password").empty()) {
		server.set_credentials(config.get("user"), config.get("password"));
		std::cout << "Credentials are set" << std::endl;
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
		st_server->clear_endpoints();
		st_server->close();
	}

	exit(0);
}
