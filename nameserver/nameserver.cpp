#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "utils.h"

static const int INT_SIZE = 4;
static const int MAX_MESSAGE_SIZE = 256;

int run_server(Info* info, RoundRobin* rr, Geography* geo, int queue_size = 10);
void handle_connection(int connectionfd, ofstream& log, Info* info, RoundRobin* rr, Geography* geo, string clientIP);
void send_all(int connectionfd, const char *message);
string receive_all(int connectionfd, int size);
void make_server_sockaddr(struct sockaddr_in *addr, int port);
int get_port_number(int sockfd);
void update_header(DNSHeader* header);

int main(int argc, char **argv) {
    // Read Arguments
    Info info(argc, argv);

    // Select Mode
    RoundRobin rr;
    Geography geo;
    switch (info.getMode()) {
        case Mode::RR:
            rr = RoundRobin(info.getServers());
            break;
        case Mode::GEO:
            geo = Geography(info.getServers());
            break;
        default:
            std::cerr << "Fail to Recognize Load Balancer" << std::endl;
            exit(1);
    }

    // Run Name Server
    run_server(&info, &rr, &geo);
    return 0;
}

/**
 * Endlessly runs a server that listens for connections and serves
 * them _synchronously_.
 */
int run_server(Info* info, RoundRobin* rr, Geography* geo, int queue_size) {
    // Open Logfile
    ofstream log(info->getLog());
    if (!log.is_open()) {
        std::cerr << "Fail to Open Logfile " << info->getLog() << std::endl;
    }

	// Create socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		std::cerr << "Error opening stream socket" << std::endl;
		exit(1);
	}

	// Set the "reuse port" socket option
	int yesval = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yesval, sizeof(yesval)) == -1) {
		std::cerr << "Error setting socket options" << std::endl;
		exit(1);
	}

	// Create a sockaddr_in struct for the proper port and bind() to it.
	struct sockaddr_in addr;
	make_server_sockaddr(&addr, info->getPort());

	// Bind to the port.
	if (bind(sockfd, (sockaddr *) &addr, sizeof(addr)) == -1) {
		std::cerr << "Error binding stream socket" << std::endl;
		exit(1);
	}

	// Detect which port was chosen.
	int port = get_port_number(sockfd);
	std::cout << "Server listening on port " << port << "..." << std::endl;

	// Begin listening for incoming connections.
	listen(sockfd, queue_size);

	// Serve incoming connections one by one forever.
	while (true) {
		int connectionfd = accept(sockfd, 0, 0);
		if (connectionfd == -1) {
			std::cerr << "Error accepting connection" << std::endl;
			exit(1);
		}

		// Get Client Address
		struct sockaddr_in client_addr;
		socklen_t addrlen = sizeof(client_addr);
		if (getpeername(connectionfd, (struct sockaddr*) &client_addr, &addrlen) == -1) {
			std::cerr << "Error getting client address" << std::endl;
			exit(1);
		}
		string ip = inet_ntoa(client_addr.sin_addr);
		std::cout << "Server connected to client " << ip << "..." << std::endl;

		handle_connection(connectionfd, log, info, rr, geo, ip);

		std::cout << "Server finished serving client " << ip << "..." << std::endl;
	}
}

/**
 * Receives DNS Header and DNS Question, and Answers DNS Header and DNS Record.
 */
void handle_connection(int connectionfd, ofstream& log, Info* info, RoundRobin* rr, Geography* geo, string clientIP) {
	std::cout << "New connection " << connectionfd << std::endl;

	// Receive DNS Header Size
	int headerSize;
	recv(connectionfd, &headerSize, sizeof(headerSize), 0);
	headerSize = ntohl(headerSize);

	// Receive DNS Header
	DNSHeader header = DNSHeader::decode(receive_all(connectionfd, headerSize));

	std::cout << "Successfully Received DNS Header" << std::endl;

	// Receive DNS Question Size
	int questionSize;
	recv(connectionfd, &questionSize, sizeof(questionSize), 0);
	questionSize = ntohl(questionSize);

	// Receive DNS Question
	DNSQuestion question = DNSQuestion::decode(receive_all(connectionfd, questionSize));
	string domain = question.QNAME;

	std::cout << "Successfully Received DNS Question" << std::endl;

	// TODO: Check QNAME is video.cse.umich.edu

	// Find the Response IP
	string ip = selectServer(info, rr, geo, clientIP);

	// Write Logfile
	log << clientIP << " " << domain << " " << ip << std::endl;

	// Edit DNS Header
	update_header(&header);
	string responseHeader = DNSHeader::encode(header);

	// Send DNS Header Size
	headerSize = htonl(sizeof(responseHeader));
	send(connectionfd, static_cast<void*>(&headerSize), sizeof(headerSize), 0);

	// Send DNS Header
	send_all(connectionfd, responseHeader.c_str());

	std::cout << "Successfully Sent DNS Header" << std::endl;
	std::cout << sizeof(responseHeader) << std::endl << responseHeader << std::endl;

	// Edit DNS Record
	DNSRecord record;
	record.TYPE = 1;
	record.CLASS = 1;
	record.TTL = 0;
	strcpy(record.NAME, domain.c_str());
	strcpy(record.RDATA, ip.c_str());
	record.RDLENGTH = static_cast<ushort>(ip.length());
	string responseRecord = DNSRecord::encode(record);

	// Send DNS Record Size
	int recordSize = htonl(sizeof(responseRecord));
	send(connectionfd, static_cast<void*>(&recordSize), sizeof(recordSize), 0);

	// Send DNS Record
	send_all(connectionfd, responseRecord.c_str());

	std::cout << "Successfully Sent DNS Record" << std::endl;
	std::cout << sizeof(responseRecord) << std::endl << responseRecord << std::endl;

	// Close connection
    close(connectionfd);
}

/**
 * Sends a string message to the server.
 */
void send_all(int connectionfd, const char *message) {	
	// Send message to remote server
	// Call send() enough times to send all the data
	size_t message_len = strlen(message);
	size_t sent = 0;
	do {
		ssize_t sval = send(connectionfd, message + sent, message_len - sent, 0);
		if (sval == -1) {
			perror("Error sending on stream socket");
			exit(1);
		}
		sent += sval;
	} while (sent < message_len);
}

/**
 * Receives a string message from the client with given size.
 */
string receive_all(int connectionfd, int size) {
	// Initialize message with given size.
	char msg[MAX_MESSAGE_SIZE + 1];
	memset(msg, 0, sizeof(msg));

	// Call recv() enough times to consume all the data the client sends.
	size_t recvd = 0;
	ssize_t rval;
	do {
		// Receive as many additional bytes as we can in one call to recv()
		// (while not exceeding MAX_MESSAGE_SIZE bytes in total).
		rval = recv(connectionfd, msg + recvd, size - recvd, 0);
		if (rval == -1) {
			std::cerr << "Error reading stream message" << std::endl;
			exit(1);
		}
		recvd += rval;
	} while (rval > 0);  // recv() returns 0 when client closes

	return msg;
}

/**
 * Return the port number assigned to a socket.
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
 * Make a server sockaddr given a port.
 */
void make_server_sockaddr(struct sockaddr_in *addr, int port) {
	// Specify socket family.
	addr->sin_family = AF_INET;

	// Specify socket address.
	addr->sin_addr.s_addr = INADDR_ANY;

	// Set the port value.
	addr->sin_port = htons(static_cast<uint16_t>(port));
}

void update_header(DNSHeader* header) {
	header->AA = 1;
	header->RD = 0;
	header->RA = 0;
	header->Z = 0;
	header->NSCOUNT = 0;
	header->ARCOUNT = 0;
}