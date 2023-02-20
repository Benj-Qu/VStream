#include "miProxy.h"
#include "helpers.h"

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
#include <getopt.h>

void MiProxy::get_options(int argc, char *argv[]) {
    if (argc == 6 && argv[1] == "--nodns") {
        dnsMode = false;
        listenPort = atoi(argv[2]);
        wwwIp = atoi(argv[3]);
        alpha = atof(argv[4]);
        logPath = argv[5];
    } else if (argc == 7 && argv[1] == "--dns") {
        dnsMode = true;
        listenPort = atoi(argv[2]);
        dnsIp = atoi(argv[3]);
        dnsPort = atoi(argv[4]);
        alpha = atof(argv[5]);
        logPath = argv[6];
    } else {
        throw runtime_error("Error: missing or extra arguments");
    }
    return;
}

void MiProxy::init() {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1025];

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
	if (make_server_sockaddr(&addr, listenPort) == -1) {
		return -1;
	}
    if (bind(sockfd, (sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("Error binding stream socket");
		return -1;
	}
    listen(sockfd, 10);
    
    memset(&client_addr, 0, sizeof (client_addr));		
	socklen_t client_addr_len = sizeof(client_addr);
}

void MiProxy::run() {
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
