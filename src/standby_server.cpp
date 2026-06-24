#include <standby_server.hpp>
#include <iostream>
#include <cstring>
#include <base64.hpp>

using namespace uthef;

const char *AUTH_HEADER_NAME = "Authorization";

standby_server::standby_server(const char *file_name, unsigned int max_clients) 
		: http_server(file_name, max_clients), udp_client(NULL, 0) {
	udp_client.bind(true);

	if (!udp_client.is_bound()) {
		std::cerr << "Could not open UDP socket" << std::endl;
		exit(-2);
	}
}

udp_socket *standby_server::get_udp_client() {
	return &udp_client;
}

void standby_server::set_credentials(std::string user, std::string password) {
	std::string credentials = user + ":" + password;
	unsigned long b64_size = 0;
	
	char *b64 = alloc_base64_str(credentials.c_str(), credentials.size(), &b64_size);

	if (!b64) {
		std::cerr << "Base64 string allocation failed" << std::endl;
		exit(-3);
		return;
	}

	auth_header = "Basic ";
	auth_header.append(b64, b64_size);

	if (auth_header == "Basic ")
		auth_header.clear();

	free(b64);
}

void standby_server::add_endpoint(endpoint *e) {
	endpoints.emplace_back(e);
	std::cout << "Endpoint \"" << e->get_url() << "\"" << " is added" << std::endl;
}

void standby_server::parse_endpoints(settings &config) {
	endpoints.clear();

	const char *endpoint_group_prefix = "endpoints/";
	const size_t endpoint_group_prefix_len = strlen(endpoint_group_prefix);

	for (auto &group : *config.get_groups()) {
		if (strncasecmp(group.first.c_str(), endpoint_group_prefix, endpoint_group_prefix_len) != 0)
			continue;

		std::string method = config.get("method", group.first),
			authorization = config.get("auth", group.first),
			action = config.get("action", group.first);
	
		uthef::endpoint_action ep_action;
		uthef::endpoint_method ep_method;
		bool ep_auth = strcasecmp(authorization.c_str(), "true") == 0 || authorization == "1";

		if (strcasecmp(action.c_str(), "wake_redir") == 0) {
			ep_action = uthef::WAKE_REDIR;
		}
		else if (strcasecmp(action.c_str(), "run_cmd") == 0) {
			ep_action = uthef::RUN_COMMAND;
		}
		else {
			std::cerr << "Invalid action in group \"" << group.first << "\"" << std::endl;
			exit(-1);
		}
	
		if (method.empty() || strcasecmp(method.c_str(), "GET") == 0) {
			ep_method = uthef::GET;
		}
		else if (strcasecmp(method.c_str(), "POST") == 0) {
			ep_method = uthef::POST;
		}
		else {
			std::cerr << "Invalid method in group \"" << group.first << "\"" << std::endl;
			exit(-1);
		}

		std::string url = group.first.substr(endpoint_group_prefix_len - 1);

		if (url.empty())
			url = "/";

		if (ep_action == uthef::WAKE_REDIR) {
			bool is_ip_empty = config.get("remote_ip", group.first).empty(),
				 is_mac_empty = config.get("remote_mac", group.first).empty();

			if (is_ip_empty ^ is_mac_empty) {
				std::cerr << "Invalid configuration in group \"" << group.first << "\"" << std::endl;
				exit(-1);
			}

			if (is_ip_empty && is_mac_empty && config.get("redirect_url", group.first).empty()) {
				std::cerr << "Invalid configuration in group \"" << group.first << "\"" << std::endl;
				exit(-1);
			}
			
			uthef::wake_redir_endpoint *ep =
				new uthef::wake_redir_endpoint(url, ep_method, ep_auth);

			ep->set_ip(config.get("remote_ip", group.first));

			if (!is_mac_empty && !ep->set_mac(config.get("remote_mac", group.first))) {
				delete ep;
				std::cerr << "Invalid MAC address in group \"" << group.first << "\"" << std::endl;
				exit(-1);
			}

			ep->set_redirect_url(config.get("redirect_url", group.first));

			add_endpoint(ep);
		}
		else if (ep_action == uthef::RUN_COMMAND) {
			if (config.get("command", group.first).empty()) {
				std::cerr << "Invalid command in group \"" << group.first << "\"" << std::endl;
				exit(-1);
			}

			uthef::run_command_endpoint *ep = 
				new uthef::run_command_endpoint(url, ep_method, ep_auth);

			ep->set_command(config.get("command", group.first));

			add_endpoint(ep);
		}
	}
}

void standby_server::clear_endpoints() {
	endpoints.clear();
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

	if (clean_url.empty())
		clean_url = "/";

	for (auto &ep_ptr : endpoints) {
		if (!compare_url(clean_url, ep_ptr.get()->get_url().c_str()))
			continue;

		if (strcmp(endpoint::map_method(ep_ptr.get()->get_method()), request_data.method.c_str()) != 0) {
			response_data.code = 405;
			break;	
		}

		if (ep_ptr->is_authentication_required()) {
			bool unauthorized = auth_header.empty() ||
				request_data.headers.find(AUTH_HEADER_NAME) == request_data.headers.end() ||
				request_data.headers[AUTH_HEADER_NAME] != auth_header;

			if (unauthorized) {
				response_data.headers["WWW-Authenticate"] = "Basic realm=\"standby_server\"";
				response_data.code = 401;
				break;
			}
		}

		ep_ptr.get()->handle_request(*this, request_data, response_data);
		break;
	}
}

bool standby_server::compare_url(std::string &url, const char *endpoint) {
	bool result = strcasecmp(url.c_str(), endpoint) == 0;

	if (!result && strcmp(endpoint, "/") != 0 && 
			strcasecmp(url.c_str(), (std::string(endpoint) + "/").c_str()) == 0) {
		result = true;
	}

	return result;
}
