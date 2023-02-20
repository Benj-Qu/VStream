#ifndef MIPROXY_H
#define MIPROXY_H

#include <iostream>
#include <vector>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO, FD_SETSIZE macros
#include <sys/types.h>
#include <unistd.h> //close
#include <sys/socket.h>
#include <sys/select.h>

#include <netinet/in.h>

using namespace std;

class MiProxy {
   public:
    void get_options(int argc, char* argv[]);
    void init();
    void run();

   private:
    bool dns_mode;
    int listen_port;
    string www_ip;
    string dns_ip;
    int dns_port;
    float alpha;
    string log_path;

    fd_set readfds;
    vector<int> client_sockets;
    int master_socket, addrlen;
    struct sockaddr_in address;

    void init_master_socket();
    void handle_master_connection();
    void handle_client_connection(int client_socket);
};

#endif
