#ifndef MIPROXY_H
#define MIPROXY_H

#include <iostream>
#include <vector>
#include <map>
#include <chrono>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO, FD_SETSIZE macros
#include <sys/types.h>
#include <unistd.h> //close
#include <sys/socket.h>
#include <sys/select.h>

#include <netinet/in.h>

using namespace std;
using namespace std::chrono;

struct Connection {
    string client_message;
    string server_message;
    int client_socket;
    int server_socket;
    size_t server_message_len;
    time_point<chrono::steady_clock> server_conn_start;
    double current_throughput;
    vector<int> available_bitrates; // in kbps
    string no_list_message;
};

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
    map<string, Connection> clients; // <ip, Connection>
    int master_socket;

    void init_master_socket();
    void handle_master_connection();
    void handle_client_connection(Connection &conn);
    void handle_request_message(Connection &conn);
    void handle_server_connection(Connection &conn);
    void handle_response_message(Connection &conn);
    int parse_header(Connection &conn);
    void parse_xml(Connection &conn);
    void parse_bitrate(Connection &conn);
    void update_throughput(Connection &conn);
};

#endif
