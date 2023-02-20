#include "miProxy.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <string>

#include "helpers.h"

void MiProxy::get_options(int argc, char *argv[]) {
    vector<string> args(argv, argv + argc);
    if (argc == 6 && args[1] == "--nodns") {
        dns_mode = false;
        listen_port = stoi(args[2]);
        www_ip = args[3];
        alpha = stof(args[4]);
        log_path = args[5];
        cout << "dns_mode: " << dns_mode
             << "\nlisten_port: " << listen_port
             << "\nwww_ip: " << www_ip
             << "\nalpha: " << alpha
             << "\nlog_path: " << log_path << endl;
    } else if (argc == 7 && args[1] == "--dns") {
        dns_mode = true;
        listen_port = stoi(args[2]);
        dns_ip = args[3];
        dns_port = stoi(args[4]);
        alpha = stof(args[5]);
        log_path = args[6];
        cout << "dns_mode: " << dns_mode
             << "\nlisten_port: " << listen_port
             << "\ndns_ip: " << dns_ip
             << "\ndns_port: " << dns_port
             << "\nalpha: " << alpha
             << "\nlog_path: " << log_path << endl;
    } else {
        throw runtime_error("Error: missing or extra arguments");
    }
    return;
}

void MiProxy::init_master_socket() {
    // create a master socket
    master_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (master_socket <= 0) {
        throw runtime_error("socket failed");
    }

    // set master socket to allow multiple connections ,
    // this is just a good habit, it will work without this
    int yes = 1;
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        throw runtime_error("setsockopt failed");
    }

    // type of socket created
    struct sockaddr_in address;
    make_server_sockaddr(&address, listen_port);

    // bind the socket to localhost listen_port
    if (bind(master_socket, (sockaddr *)&address, sizeof(address)) < 0) {
        throw runtime_error("bind failed");
    }
    printf("---Listening on port %d---\n", listen_port);

    // try to specify maximum of 10 pending connections for the master socket
    if (listen(master_socket, 10) < 0) {
        runtime_error("listen failed");
    }
}

void MiProxy::init() {
    init_master_socket();
    puts("Waiting for connections ...");
}

void MiProxy::handle_master_connection() {
    // write new socket info to address
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int new_socket = accept(master_socket, (sockaddr *)&address, (socklen_t *)&addrlen);
    if (new_socket < 0) {
        throw runtime_error("accept");
    }

    // inform user of socket number - used in send and receive commands
    string ip = inet_ntoa(address.sin_addr);
    printf("\n---New host connection---\n");
    printf("socket fd is %d , ip is : %s , port : %d \n", new_socket,
           ip.c_str(), ntohs(address.sin_port));

    // add new socket to the array of sockets
    if (clients.find(ip) != clients.end()) {
        cout << "client already exists" << endl;
        clients[ip].client_socket = new_socket;
    } else {
        clients[ip] = {};
        clients[ip].client_socket = new_socket;
        clients[ip].server_socket = -1;
    }
}

const static int BUFFER_SIZE = 1024;

void MiProxy::handle_client_connection(Connection &conn) {
    cout << "\n---Handling client connection at socket " << conn.client_socket << "---" << endl;
    char buffer[BUFFER_SIZE];  // data buffer of 1KB
    // Check if it was for closing , and also read the incoming message
    // Returns the address in address
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    getpeername(conn.client_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    cout << "Starting to read from client socket " << conn.client_socket << endl;
    ssize_t valread = read(conn.client_socket, buffer, BUFFER_SIZE);
    cout << "Read " << valread << " bytes from client socket " << conn.client_socket << endl;

    if (valread == 0) {
        // Somebody disconnected, get their details and print
        printf("\n---Client disconnected---\n");
        printf("Client disconnected , ip %s , port %d \n",
               inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        // Close the socket and mark as 0 in list for reuse
        close(conn.client_socket);
        clients.erase(inet_ntoa(address.sin_addr));
        return;
    }

    conn.client_message.append(buffer, valread);
    if (conn.client_message.find("\r\n\r\n") != string::npos) {
        // Request message is complete
        cout << "\n---New message---\n";
        cout << conn.client_message << endl;
        printf("\nReceived from: ip %s , port %d \n",
               inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        handle_request_message(conn);
    }
}

void MiProxy::handle_request_message(Connection &conn) {
    // forward the message to the server
    if (conn.server_socket == -1) {
        // create a new socket to connect to the server
        conn.server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (conn.server_socket < 0) {
            throw runtime_error("socket failed");
        }

        struct sockaddr_in address;
        if (make_client_sockaddr(&address, www_ip.c_str(), 80) == -1) { //TODO
            throw runtime_error("make_client_sockaddr failed");
        }

        cout << "Connecting to server..." << endl;
        if (connect(conn.server_socket, (sockaddr *)&address, sizeof(address)) < 0) {
            throw runtime_error("connect failed");
        }
    }

    // send the message
    cout << "Sending message to server..." << endl;
    send_all(conn.server_socket, conn.client_message.c_str(), conn.client_message.size());
    conn.client_message.clear();
}

void MiProxy::handle_server_connection(Connection &conn) {
    cout << "\n---Handling server connection at socket " << conn.server_socket << "---" << endl;

    if (conn.server_message.empty()){
        // new message from server
        conn.server_conn_start = steady_clock::now();
    }

    char buffer[BUFFER_SIZE];  // data buffer of 1KB
    // Check if it was for closing , and also read the incoming message
    // Returns the address in address
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    getpeername(conn.server_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    cout << "Starting to read from server socket " << conn.server_socket << endl;
    ssize_t valread = read(conn.server_socket, buffer, BUFFER_SIZE);
    cout << "Read " << valread << " bytes from server socket " << conn.server_socket << endl;

    if (valread == 0) {
        // Server disconnected, get their details and print
        printf("\n---Server disconnected---\n");
        printf("Server disconnected , ip %s , port %d \n",
               inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        // Close the socket and mark as 0 in list for reuse
        close(conn.server_socket);
        conn.server_socket = -1;
        return;
    }

    conn.server_message.append(buffer, valread);

    if (conn.server_message_len == 0 && parse_header(conn) == -1) {
        return; // header not complete
    }

    if (conn.server_message.size() < conn.server_message_len) {
        cout << "Received " << conn.server_message.size() << " bytes, waiting for "
             << conn.server_message_len - conn.server_message.size() << " more bytes..." << endl;
        return;
    }

    // Request message is complete
    cout << "\n---New message---\n";
    cout << conn.server_message.substr(0, BUFFER_SIZE * 5) << endl;
    printf("\nReceived from: ip %s , port %d \n",
           inet_ntoa(address.sin_addr), ntohs(address.sin_port));
    handle_response_message(conn);
}

void MiProxy::handle_response_message(Connection &conn) {
    // forward the message to the client
    // send the message
    cout << "Sending message to client..." << endl;
    send_all(conn.client_socket, conn.server_message.c_str(), conn.server_message.size());
    // check xml file
    if (conn.available_bitrates.empty()) {
        parse_xml(conn);
    } else {
        // calculate throughput
        duration<double> time_diff = steady_clock::now() - conn.server_conn_start;
        double new_throughput = (double)conn.server_message_len / time_diff.count() * 8 / 1000; // kbps
        conn.current_throughput = alpha * new_throughput + (1 - alpha) * conn.current_throughput;
        cout << "Time diff: " << time_diff.count() << " s" << endl;
        cout << "New throughput: " << new_throughput << " kbps" << endl;
        cout << "Current throughput: " << conn.current_throughput << " kbps" << endl;
    }

    conn.server_message.clear();
    conn.server_message_len = 0;
}

void MiProxy::parse_xml(Connection &conn) {
    // check content type
    if (conn.server_message.find("Content-Type: text/xml") == string::npos) {
        cout << "---Not a text xml---" << endl;
        return;
    }
    cout << "---Parsing xml---" << endl;
    size_t br_pos, br_end_pos;
    while ((br_pos = conn.server_message.find("bitrate=\"")) != string::npos) {
        br_end_pos = conn.server_message.find("\"", br_pos + 9);
        string br_str = conn.server_message.substr(br_pos + 9, br_end_pos - br_pos - 9);
        conn.available_bitrates.push_back(stoi(br_str));
        conn.server_message.erase(br_pos, br_end_pos - br_pos + 1);
        cout << "Available bitrate: " << br_str << endl;
    }
    sort(conn.available_bitrates.begin(), conn.available_bitrates.end());
    conn.current_throughput = conn.available_bitrates[0] * 1.5;
    cout << "initialize current_throughput: " << conn.current_throughput << endl;
}

int MiProxy::parse_header(Connection &conn) {
    size_t end_pos = conn.server_message.find("\r\n\r\n");
    if (end_pos == string::npos) {
        cout << "header not complete" << endl;
        return -1;
    }
    size_t cl_pos = conn.server_message.find("Content-Length: ");
    size_t con_end_pos = conn.server_message.find("\r\n", cl_pos);
    if (cl_pos == string::npos || con_end_pos == string::npos) {
        cout << "---No Content-Length header---" << endl;
        cout << conn.server_message << endl;
        return -1;
    }
    string cl_str = conn.server_message.substr(cl_pos + 16, con_end_pos - cl_pos - 16);
    int content_length = stoi(cl_str);
    cout << "Content-Length: " << content_length << endl;
    conn.server_message_len = end_pos + 4 + (size_t)content_length;
    return 0;
}

void MiProxy::run() {
    while (true) {
        // clear the socket set
        FD_ZERO(&readfds);

        // add master socket to set
        FD_SET(master_socket, &readfds);
        // add client and server sockets to set
        cout << "Number of client sockets: " << clients.size() << endl;
        for (auto client : clients) {
            FD_SET(client.second.client_socket, &readfds);
            if (client.second.server_socket != -1) {
                FD_SET(client.second.server_socket, &readfds);
            }
        }
        cout << "Waiting for activity on sockets..." << endl;
        // wait for an activity on one of the sockets, timeout is NULL,
        // so wait indefinitely
        ssize_t activity = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
        cout << "Activity detected on socket!" << endl;
        if ((activity < 0) && (errno != EINTR)) {
            throw runtime_error("select error");
        }

        // If something happened on the master socket,
        // then its an incoming connection, call accept()
        if (FD_ISSET(master_socket, &readfds)) {
            handle_master_connection();
        }
        // else it's some IO operation on a client socket
        for (auto &client : clients) {
            if (FD_ISSET(client.second.client_socket, &readfds)) {
                handle_client_connection(client.second);
            } else if (client.second.server_socket != -1 &&
                       FD_ISSET(client.second.server_socket, &readfds)) {
                handle_server_connection(client.second);
            }
        }
    }
}
