#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "utils.h"

static const int MAX_MESSAGE_SIZE = 256;



/**
 * Return the port number assigned to a socket.
 *
 * Parameters:
 * 		sockfd:	File descriptor of a socket
 *
 * Returns:
 *		The port number of the socket, or -1 on failure.
 */
int get_port_number(int sockfd) {
    struct sockaddr_in addr;
	socklen_t length = sizeof(addr);
	if (getsockname(sockfd, (sockaddr *) &addr, &length) == -1) {
		perror("Error getting port of socket");
		return -1;
	}
	// Use ntohs to convert from network byte order to host byte order.
	return ntohs(addr.sin_port);
}

/**
 * Sends a string message to the server.
 *
 * Parameters:
 *		hostname: 	Remote hostname of the server.
 *		port: 		Remote port of the server.
 * 		message: 	The message to send, as a C-string.
 * Returns:
 *		0 on success, -1 on failure.
 */
int send_message(const char *hostname, int port, const char *message) {
	if (strlen(message) > MAX_MESSAGE_SIZE) {
		perror("Error: Message exceeds maximum length\n");
		return -1;
	}

	// (1) Create a socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	// (2) Create a sockaddr_in to specify remote host and port
	struct sockaddr_in addr;
	if (make_client_sockaddr(&addr, hostname, port) == -1) {
		return -1;
	}

	// (3) Connect to remote server
	if (connect(sockfd, (sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("Error connecting stream socket");
		return -1;
	}
	
	// (4) Send message to remote server
	// Call send() enough times to send all the data
	size_t message_len = strlen(message);
	size_t sent = 0;
	do {
		ssize_t n = send(sockfd, message + sent, message_len - sent, 0);
		if (n == -1) {
			perror("Error sending on stream socket");
			return -1;
		}
		sent += n;
	} while (sent < message_len);

	// (5) Close connection
	close(sockfd);

	return 0;
}

/**
 * Receives a string message from the client and prints it to stdout.
 *
 * Parameters:
 * 		connectionfd: 	File descriptor for a socket connection
 * 				(e.g. the one returned by accept())
 * Returns:
 *		0 on success, -1 on failure.
 */
int handle_connection(int connectionfd) {

	printf("New connection %d\n", connectionfd);

	// (1) Receive message from client.

	char msg[MAX_MESSAGE_SIZE + 1];
	memset(msg, 0, sizeof(msg));

	// Call recv() enough times to consume all the data the client sends.
	size_t recvd = 0;
	ssize_t rval;
	do {
		// Receive as many additional bytes as we can in one call to recv()
		// (while not exceeding MAX_MESSAGE_SIZE bytes in total).
		rval = recv(connectionfd, msg + recvd, MAX_MESSAGE_SIZE - recvd, 0);
		if (rval == -1) {
			perror("Error reading stream message");
			return -1;
		}
		recvd += rval;
	} while (rval > 0);  // recv() returns 0 when client closes

	// (2) Print out the message
	printf("Client %d says '%s'\n", connectionfd, msg);

	// (4) Close connection
	close(connectionfd);

	return 0;
}

#endif