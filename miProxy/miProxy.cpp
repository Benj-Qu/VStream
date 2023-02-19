#include <iostream>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/select.h>
#include <vector>
#include <algorithm>
#include <cassert>
#include <sys/time.h> 
#include <errno.h>
#include <cstdio>
#include <vector>

int main(int argc, char*argv[])
{
    int listen_port;
    bool www_ip=0;
    int alpha;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1025];
    if (argc ==6)
    {
        listen_port = atoi(argv[2]);
        www_ip = atoi(argv[3]);
        alpha = atoi(argv[4]);
    }

    struct sockaddr_in client_addr, server_addr;
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == -1) {
		perror("Error opening stream socket");
		return -1;
	}
    int yesval = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yesval, sizeof(yesval)) == -1) {
		perror("Error setting socket options");
		return -1;
	}
    struct sockaddr_in addr;
	if (make_server_sockaddr(&addr, listen_port) == -1) {
		return -1;
	}
    if (bind(sockfd, (sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("Error binding stream socket");
		return -1;
	}
    listen(sockfd, 10);
    
    memset(&client_addr, 0, sizeof (client_addr));		
	socklen_t client_addr_len = sizeof(client_addr);

    fd_set readfds;
    std::vector<int> client_socket;
    while(true)
    {
        int maxfd = 0;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        int activity = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
        if (activity <  0) {
            perror("select error");
        }

        if (FD_ISSET(sockfd, &readfds)) {
            int new_socket = accept(sockfd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
            if(new_socket < 0) {
                perror("accept error");
            }
            client_socket.push_back(new_socket);
        }
        for(auto &client_sock: client_socket){
            if (client_sock != 0 && FD_ISSET(client_sock, &readfds)){
                getpeername(client_sock, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                int val = recv(client_sock, buffer, 1025, 0);
                if(val == 0) {
                    close(client_sock);
                    client_sock = 0;
                }
                else {
                    int web_server_port = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                    struct sockaddr_in addr;
                    if (connect(sockfd, (sockaddr *) &addr, sizeof(addr)) == -1) {
                        perror("Error connecting stream socket");
                        return -1;
                    }
                }
            }
        }
    }
    
}

int make_client_sockaddr(struct sockaddr_in *addr, const char *hostname, int port) {
	// Step (1): specify socket family.
	// This is an internet socket.
	addr->sin_family = AF_INET;

	// Step (2): specify socket address (hostname).
	// The socket will be a client, so call this unix helper function
	// to convert a hostname string to a useable `hostent` struct.
	struct hostent *host = gethostbyname(hostname);
	if (host == nullptr) {
		fprintf(stderr, "%s: unknown host\n", hostname);
		return -1;
	}
	memcpy(&(addr->sin_addr), host->h_addr, host->h_length);

	// Step (3): Set the port value.
	// Use htons to convert from local byte order to network byte order.
	addr->sin_port = htons(port);

	return 0;
}

int make_server_sockaddr(struct sockaddr_in *addr, int port) {
	// Step (1): specify socket family.
	// This is an internet socket.
	addr->sin_family = AF_INET;

	// Step (2): specify socket address (hostname).
	// The socket will be a server, so it will only be listening.
	// Let the OS map it to the correct address.
	addr->sin_addr.s_addr = INADDR_ANY;

	// Step (3): Set the port value.
	// If port is 0, the OS will choose the port for us.
	// Use htons to convert from local byte order to network byte order.
	addr->sin_port = htons(port);

	return 0;
}