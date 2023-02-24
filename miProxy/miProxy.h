#ifndef MIPROXY_H
#define MIPROXY_H

#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>  //FD_SET, FD_ISSET, FD_ZERO, FD_SETSIZE macros
#include <sys/types.h>
#include <unistd.h>  //close

#include <chrono>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>

#include "DNSHeader.h"
#include "DNSQuestion.h"
#include "DNSRecord.h"

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
    vector<int> available_bitrates;  // in kbps
    string no_list_message;
    string chunkname;
    string server_ip;
    int server_port;
    string client_ip; // also used as key in clients map
    int current_bitrate;
};

class MiProxy {
   public:
    void get_options(int argc, char *argv[]);
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
    map<string, Connection> clients;  // <client_ip, Connection>
    int master_socket;
    ofstream log;
    int dns_socket;

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
    
    void init_dns_socket();
    void handle_dns_request();
    void handle_dns_response();
    string make_dns_Header();
    string make_dns_Question();

    string receive_all(int connectionfd, uint32_t size);


};

#endif
