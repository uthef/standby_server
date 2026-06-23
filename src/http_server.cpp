#include <http_server.hpp>
#include <curl/curl.h>
#include <stdint.h>

using namespace uthef;

http_server::http_server(const char *file_name, unsigned int max_clients) 
	: unix_tcp_server(file_name, max_clients) {
	
}

void http_server::handle_client(int sockd) {
	char buffer[4097];
	int bytes_read = read(sockd, buffer, sizeof(buffer));
	std::string response_buffer;
	response response_data;
	response_data.code = 0;

	if (bytes_read < 0)
		return;
	
	if (bytes_read < sizeof(buffer))
		buffer[bytes_read] = 0;
	
	request request_data;

	if (bytes_read == 0 || bytes_read >= sizeof(buffer)) {
		response_data.code = 400;
	}
	else if (!parse_request(buffer, bytes_read, request_data)) {
		response_data.code = 400;
	}
	else {
		handle_request(request_data, response_data);
	}

	if (response_data.code == 0) {
		response_data.code = 404;
	}
	
	if (response_data.body.empty() && response_data.message.empty()) {
		std::string message = message_for_status(response_data.code);
		response_data.body = message;
		response_data.message = message;
	}

	std::stringstream response_stream;
	write_response(response_stream, response_data);

	response_buffer = response_stream.str();

	send(sockd, response_buffer.c_str(), response_buffer.size());
}

void http_server::handle_request(request &request_data, response &response_data) {
	response_data.code = 404;
}

void http_server::decode_url(std::string &str) {
	int unescaped_url_length = 0;

	char *unescaped_url = 
		curl_easy_unescape(NULL, str.c_str(), str.size(), &unescaped_url_length);

	str = std::string(unescaped_url, unescaped_url_length);

	curl_free(unescaped_url);
}

bool http_server::parse_request(const char *buffer, size_t buffer_size, request &out_request) {
	size_t pos = 0;
	int line_breaks = 0, state = 0;

	std::string value1, value2, http_version;

	for (pos = 0; pos < buffer_size; pos++) {
		char c = buffer[pos];
		
		if (c == '\r' || (c == ' ' && value1.empty()) )
			continue;

		if (c == '\n')
			break;

		if (c == ' ') {
			if (state == 0) 
				out_request.method = value1;
			else if (state == 1)
				out_request.url = value1;
			
			if (state < 2)
				value1.clear();
			else 
				value1.push_back(c);

			state++;	
			continue;
		}

		value1.push_back(c);
	}

	http_version = value1;

	if (out_request.method != "GET" && out_request.method != "POST")
		return false;

	if (out_request.url.empty() || out_request.url.front() != '/')
		return false;
	
	size_t query_start_pos = out_request.url.find('?');

	if (query_start_pos != SIZE_MAX) {
		out_request.query = out_request.url.substr(query_start_pos + 1);
		out_request.url = out_request.url.substr(0, query_start_pos);
	}

	decode_url(out_request.url);
	decode_url(out_request.query);

	if (strncasecmp(http_version.c_str(), "HTTP/", 5) != 0)
		return false;

	value1.clear();
	state = 0;

	for (; pos < buffer_size; pos++) {
		char c = buffer[pos];

		if (c == '\r' || (c == ' ' && value1.empty())) {
			continue;
		}

		if (c == '\n') {
			line_breaks++;

			if (line_breaks >= 2) {
				pos++;
				break;
			}

			if (!value1.empty())
				out_request.headers[value1] = value2;

			value1.clear();
			value2.clear();
			state = 0;
			continue;
		}	

		line_breaks = 0;

		if (state == 0 && c == ':') {
			state = 1;
			continue;
		}

		if (c == ' ' && state == 1 && value2.empty()) {
			continue;
		}

		if (state == 0)
			value1.push_back(c);
		else if (state == 1)
			value2.push_back(c);
	}

	int content_length = buffer_size - pos;
	auto length_header = out_request.headers.find("Content-Length");

	if (length_header != out_request.headers.end()) {
		try {
			content_length = std::stoi(length_header->second);
		}
		catch (const std::exception &e) {
			return false;
		}

		if (content_length < 0 || pos + content_length > buffer_size)
			return false;
	}

	if (pos + content_length <= buffer_size) {
		out_request.body.append(buffer, pos, content_length);
	}

	return true;
}

void http_server::write_response(std::stringstream &stream, response &response_data) {
	bool type_defined = false;

	stream << "HTTP/1.0 " << response_data.code;

	if (!response_data.message.empty()) {
		stream << " " << response_data.message;
	}

	stream << std::endl;

	for (auto &header : response_data.headers) {
		if (!type_defined && header.first.size() == 12) {
			type_defined = strncasecmp(header.first.c_str(), "content-type", header.first.size()) == 0;
		}

		stream << header.first << ": " << header.second << std::endl;
	}

	if (!response_data.body.empty() && !type_defined)
		stream << "Content-Type: text/plain" << std::endl;
	
	if (!response_data.body.empty())
		stream << "Content-Length: " << response_data.body.size() << std::endl;

	stream << std::endl;
	
	if (response_data.body.empty())
		return;	

	stream << response_data.body;
	stream << std::endl;
}

std::string http_server::message_for_status(int code) {
	switch (code) {
		case 200:
			return "200 OK";
		case 302:
			return "302 Termporarily Moved";
		case 400:
			return "400 Bad Request";
		case 401:
			return "401 Unauthorized";
		case 405:
			return "405 Method Not Allowed";
		case 404:
			return "404 Not Found";
		case 500:
			return "500 Internal Server Error";
	}	
	
	std::stringstream ss;
	ss << code;

	return ss.str() + " Other";
}
