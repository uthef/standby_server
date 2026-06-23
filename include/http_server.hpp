#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <unix_tcp_server.hpp>
#include <sstream>
#include <unordered_map>

namespace uthef {

class http_server : public unix_tcp_server {
protected:
	typedef std::unordered_map<std::string, std::string> header_map;
public:
	struct request {
		std::string method;
		std::string url;
		std::string query;
		header_map headers;
		std::string body;
	};

	struct response {
		int code = 0;
		std::string message;
		header_map headers;
		std::string body;
	};

	http_server(const char *file_name, unsigned int max_clients = 128);
protected:
	void handle_client(int sockd) override;
	virtual void handle_request(request &request_data, response &response_data);
	static void decode_url(std::string &str);
private:
	bool parse_request(const char *buffer, size_t buffer_size, request &out_request);
	void write_response(std::stringstream &stream, response &response_data);

	static std::string message_for_status(int code);
};	

}

#endif
