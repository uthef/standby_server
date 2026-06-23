#include <unix_tcp_server.hpp>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>
#include <cstring>
#include <stdio.h>
#include <poll.h>

using namespace uthef;

unix_tcp_server::unix_tcp_server(const char *file_name, unsigned int max_clients) {
	memset(&addr, 0, sizeof(addr));
	this->max_clients = max_clients;

	addr.sun_path[0] = 0;
	addr.sun_family = AF_UNIX;

	if (!file_name)
		return;

	strcpy(addr.sun_path, file_name);
}

unix_tcp_server::unix_tcp_server(unix_tcp_server &&other) {
	move(other);
}

unix_tcp_server::~unix_tcp_server() {
	close();
}

unix_tcp_server &unix_tcp_server::operator =(unix_tcp_server &&other) {
	return move(other);
}

bool unix_tcp_server::is_listening() const {
	return listening;
}	

void unix_tcp_server::listen(const int backlog) {
	if (!*addr.sun_path)
		return;

	if (listening)
		close();

	sockd = socket(AF_UNIX, SOCK_STREAM, 0);

	if (sockd == -1)
		return;
	
	unlink(addr.sun_path);

	int result = bind(sockd, (sockaddr *)&addr, sizeof(addr));	

	if (result) {
		::close(sockd);
		return;
	}

	result = ::listen(sockd, backlog);
	listening = true;

	if (result) {
		close();
		return;
	}

	printf("TCP socket \"%s\" is open\n", addr.sun_path);
}

void unix_tcp_server::poll() {
	if (!listening || polling)
		return;

	const int max_fds = max_clients + 1;
	int fds_count = 1;

	struct pollfd *fds = new struct pollfd[max_fds];

	if (!fds)
		return;

	fds[0].fd = sockd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	polling = true;

	while (polling) {
		::poll(fds, max_fds, 1000);

		for (int i = 0; i < fds_count; i++) {
			struct pollfd pfd = fds[i];

			if (!(pfd.revents & POLLIN))
				continue;

			if (pfd.fd == sockd) {
				struct sockaddr client_addr;
				socklen_t addr_size = sizeof(client_addr);

				int client_socket = ::accept(sockd, &client_addr, &addr_size);

				if (client_socket == -1)
					continue;

				if (fds_count == max_fds) {
					::close(client_socket);
					continue;
				}

				fds[fds_count].fd = client_socket;
				fds[fds_count].events = POLLIN;
				fds[fds_count].revents = 0;

				fds_count++;

				continue;
			}
			
			handle_client(pfd.fd);

			::close(pfd.fd);

			*(fds + i) = *(fds + (fds_count - 1));
			fds_count--;
			i--;
			continue;
			
		}
	}

	delete[] fds;
}

void unix_tcp_server::handle_client(int sockd) {

}

int unix_tcp_server::read(int sockd, char *buffer, size_t buffer_size) {
	return recv(sockd, buffer, buffer_size, 0);
}

int unix_tcp_server::send(int sockd, const char *buffer, size_t buffer_size) {
	return ::send(sockd, buffer, buffer_size, 0);
}

void unix_tcp_server::stop_polling() {
	polling = false;
}

void unix_tcp_server::close() {
	stop_polling();

	if (!listening)
		return;

	::close(sockd);
	unlink(addr.sun_path);

	listening = false;
	printf("TCP socket \"%s\" is closed\n", addr.sun_path);
}

unix_tcp_server &unix_tcp_server::move(unix_tcp_server &other) {
	close();

	sockd = std::move(other.sockd);
	listening = std::move(other.listening);
	addr = std::move(other.addr);
	
	other.stop_polling();
	other.listening = false;

	return *this;
}
