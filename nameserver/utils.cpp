#include "utils.h"

#define CACHE_SIZE 32

// Info

Info::Info(int argc, char** argv) {
    // Check Argument Number
    if (argc != 5) {
        usage();
    }
    // Check and Read Load Balancer Mode
    if (strcmp(argv[1], "--rr") == 0) {
        this->mode = Mode::RR;
    }
    else if (strcmp(argv[1], "--geo") == 0) {
        this->mode = Mode::GEO;
    }
    else {
        usage();
    }
    // Read Port, Servers, and Log
    this->port = atoi(argv[2]);
    this->servers = argv[3];
    this->log = argv[4];
}

void Info::usage() {
    std::cerr << "./nameserver [--geo|--rr] <port> <servers> <log>" << std::endl;
    exit(1);
}


// Round Robin

RoundRobin::RoundRobin(string filename) {
    // Set Index
    this->index = 0;
    // Open File
    ifstream file(filename);
    // Check File Status
    if (!file.is_open()) {
        std::cerr << "Fail to Open File: " << filename << std::endl;
        exit(1);
    }
    // Read Ip Address
    string ip;
    while (file >> ip) {
        hosts.push_back(ip);
    }
    // Close File
    file.close();
}

string RoundRobin::next() {
    int idx = this->index;
    this->index = (this->index + 1) % static_cast<int>(this->hosts.size());
    return this->hosts[idx];
}


// Geography

Node::Node(string _type, string _ip) {
    // Set Ip
    this->ip = _ip;
    // Check and Set Host Type
    if (_type == "CLIENT") {
        this->type = NodeType::CLIENT;
    }
    else if (_type == "SWITCH") {
        this->type = NodeType::SWITCH;
    }
    else if (_type == "SERVER") {
        this->type = NodeType::SERVER;
    }
    else {
        std::cerr << "Fail to Recognize Host Type: " << _type << std::endl;
        exit(1);
    }
}

bool MapComp(const pair<Node*, int>& lhs, const pair<Node*, int>& rhs) {
    return (lhs.second < rhs.second);
}

Geography::Geography(string filename) {
    // Open File
    ifstream file(filename);
    // Junk Container
    string s;
    // Read Node Number
    int NodeNum = 0;
    file >> s >> NodeNum;
    // Read Nodes
    for (int i = 0; i < NodeNum; i++) {
        int id = 0;
        string type, ip;
        file >> id >> type >> ip;
        Node* nptr = new Node(type, ip);
        // Check Duplicate Id and Ip
        if ((this->nodes.find(id) == this->nodes.end()) && (this->IPmap.find(ip) == this->IPmap.end())) {
            this->nodes[id] = nptr;
            this->IPmap[ip] = nptr;
        }
        else {
            std::cerr << "Duplicate Node with id " << id << "or ip " << ip << std::endl;
            exit(1);
        }
    }
    // Read Edge Number
    int EdgeNum = 0;
    file >> s >> EdgeNum;
    // Read Edges
    for (int i = 0; i < EdgeNum; i++) {
        int id1, id2, dist;
        file >> id1 >> id2 >> dist;
        Node* nptr1 = this->nodes[id1];
        Node* nptr2 = this->nodes[id2];
        // Mark Node 2 as the Neighbor of Node 1
        if (this->edges.find(nptr1) == this->edges.end()) {
            this->edges[nptr1] = {Neighbor(nptr2, dist)};
        }
        else {
            this->edges[nptr1].push_back(Neighbor(nptr2, dist));
        }
        // Mark Node 1 as the Neighbor of Node 2
        if (this->edges.find(nptr2) == this->edges.end()) {
            this->edges[nptr2] = {Neighbor(nptr1, dist)};
        }
        else {
            this->edges[nptr2].push_back(Neighbor(nptr1, dist));
        }
    }
    // Close file
    file.close();
}

Geography::~Geography() {
    for (pair<int, Node*> node : this->nodes) {
        delete node.second;
    }
}

string Geography::findServer(string client) {
    // Check Client in network
    if (this->IPmap.find(client) == this->IPmap.end()) {
        std::cerr << "Fail to Find Client with IP " << client << std::endl;
        exit(1);
    }
    // Check Cache for client
    if (this->cache.find(client) != this->cache.end()) {
        return this->cache[client];
    }
    // Find Nearest Server from Client
    Neighbor current(this->IPmap[client], 0);
    map<Node*, int> distMap = {{this->IPmap[client], 0}};
    unordered_set<Node*> visited;
    // Traverse All Nodes
    while (!distMap.empty()) {
        // Return Current Node if Server
        if (current.getNptr()->getType() == NodeType::SERVER) {
            // Dump Cache if Full
            while (cache.size() >= CACHE_SIZE) {
                cache.erase(cache.begin());
            }
            // Add Client Server Map to Cache
            cache[client] = current.getNptr()->getIp();
            return current.getNptr()->getIp();
        }
        // Mark Current Node Visited
        visited.insert(current.getNptr());
        // Remove Current Node from DistMap
        distMap.erase(current.getNptr());
        // Record Distance of Unvisited Neighbors of Current Node
        for (Neighbor neighbor : this->edges[current.getNptr()]) {
            // Skip Visited Nodes and Clients
            if ((visited.find(neighbor.getNptr()) != visited.end()) || (neighbor.getNptr()->getType() == NodeType::CLIENT)) {
                continue;
            }
            // If Neighbor Not Found Yet
            if (distMap.find(neighbor.getNptr()) == distMap.end()) {
                distMap[neighbor.getNptr()] = current.getDistance() + neighbor.getDistance();
            }
            // If Neighbor Already Found
            else {
                distMap[neighbor.getNptr()] = min(distMap[neighbor.getNptr()], current.getDistance() + neighbor.getDistance());
            }
        }
        // Iterate to Nearest Neighbor
        current = Neighbor(*min_element(distMap.begin(), distMap.end(), MapComp));
    }
    // No Connected Server Found
    std::cerr << "Fail to Find Connected Server from Client with IP " << client << std::endl;
    exit(1);
}