#include "miProxy.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <string>

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
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(listen_port);

    addrlen = sizeof(address);

    // bind the socket to localhost listen_port
    if (bind(master_socket, (sockaddr *)&address, addrlen) < 0) {
        throw runtime_error("bind failed");
    }
    printf("---Listening on port %d---\n", listen_port);

    // try to specify maximum of 10 pending connections for the master socket
    if (listen(master_socket, 10) < 0) {
        runtime_error("listen failed");
    }
}

void MiProxy::init() {
    client_sockets.clear();
    init_master_socket();
    puts("Waiting for connections ...");
}

void MiProxy::handle_master_connection() {
    int new_socket = accept(master_socket, (sockaddr *)&address, (socklen_t *)&addrlen);
    if (new_socket < 0) {
        throw runtime_error("accept");
    }

    // inform user of socket number - used in send and receive commands
    printf("\n---New host connection---\n");
    printf("socket fd is %d , ip is : %s , port : %d \n", new_socket,
           inet_ntoa(address.sin_addr), ntohs(address.sin_port));

    // send new connection greeting message
    // TODO: REMOVE THIS CALL TO SEND WHEN DOING THE ASSIGNMENT 2.
    // ssize_t send_rval = send(new_socket, message, strlen(message), 0);
    // if (send_rval != strlen(message)) {
    //     perror("send");
    // }
    // printf("Welcome message sent successfully\n");

    // add new socket to the array of sockets
    client_sockets.push_back(new_socket);
}

void MiProxy::handle_client_connection(int client_sock) {
    char buffer[1025];  // data buffer of 1KiB + 1 bytes
    // Check if it was for closing , and also read the
    // incoming message
    getpeername(client_sock, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    ssize_t valread = read(client_sock, buffer, 1024);
    if (valread == 0) {
        // Somebody disconnected, get their details and print
        printf("\n---Client disconnected---\n");
        printf("Client disconnected , ip %s , port %d \n",
               inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        // Close the socket and mark as 0 in list for reuse
        close(client_sock);
        client_sockets.erase(remove(client_sockets.begin(), client_sockets.end(), client_sock),
                             client_sockets.end());
    } else {
        // send the same message back to the client, hence why it's called
        // "echo_server"
        buffer[valread] = '\0';
        printf("\n---New message---\n");
        printf("Message %s\n", buffer);
        printf("Received from: ip %s , port %d \n",
               inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        // TODO: WHEN DOING ASSIGNMENT 2 REMEMBER TO REMOVE THIS LINE!
        // send(client_sock, buffer, strlen(buffer), 0);
    }
}

void MiProxy::run() {
    while (true) {
        // clear the socket set
        FD_ZERO(&readfds);

        // add master socket to set
        cout << "Adding master socket to set..." << endl;
        FD_SET(master_socket, &readfds);
        // add client sockets to set
        cout << "Adding client sockets to set..." << endl;
        cout << "Number of client sockets: " << client_sockets.size() << endl;
        for (int client_sock : client_sockets) {
            FD_SET(client_sock, &readfds);
            cout << "Adding client socket " << client_sock << " to set..." << endl;
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
        for (int client_sock : client_sockets) {
            if (FD_ISSET(client_sock, &readfds)) {
                handle_client_connection(client_sock);
            }
        }
    }
}
