#ifndef UNIX_TCP_SERVER_HPP
#define UNIX_TCP_SERVER_HPP

#include <sys/un.h>

namespace uthef {

class unix_tcp_server {
private:
	bool listening = false,
		 polling = false;
	int sockd = 0;
	unsigned int max_clients = 128;
	struct sockaddr_un addr;
public:
	unix_tcp_server(const char *file_name, unsigned int max_clients = 128);
	unix_tcp_server(const unix_tcp_server &) = delete;
	unix_tcp_server(unix_tcp_server &&);
	~unix_tcp_server();

	unix_tcp_server &operator =(const unix_tcp_server &) = delete;
	unix_tcp_server &operator =(unix_tcp_server &&);
	
	bool is_listening() const;
	void listen(const int backlog = 5);
	void poll();
	int read(int sockd, char *buffer, size_t buffer_size);
	int send(int sockd, const char *buffer, size_t buffer_size);
	void stop_polling();
	void close();
protected:
	virtual void handle_client(int sockd);
private:
	unix_tcp_server &move(unix_tcp_server &);
};

}

#endif
