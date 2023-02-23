#include "utils.h"


#define CACHE_SIZE 32


// Select Server

string selectServer(Info* info, RoundRobin* rr, Geography* geo, string client) {
    switch (info->getMode()) {
        case Mode::RR:
            return rr->next();
            break;
        case Mode::GEO:
            return geo->findServer(client);
            break;
        default:
            std::cerr << "Fail to Recognize Load Balancer" << std::endl;
            exit(1);
    }
}

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
    std::cerr << "Usage: ./nameserver [--geo|--rr] <port> <servers> <log>" << std::endl;
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

Node::Node(int _id, string _type, string _ip) {
    // Set Id
    this->id = _id;
    // Set Ip
    this->ip = IP_UINT(_ip);
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

uint32_t IP_UINT(string IP) {
    if (IP == "NO_IP") {
        return 0;
    }
    std::istringstream iss(IP);
    char s;
    uint32_t ips[4];
    memset(ips, 0, 4 * sizeof(uint32_t));
    iss >> ips[0] >> s >> ips[1] >> s >> ips[2] >> s >> ips[3];
    return (ips[0] << 24) + (ips[1] << 16) + (ips[2] << 8) + ips[3];
}

string UINT_IP(uint32_t IP) {
    std::ostringstream oss;
    uint32_t ips[4];
    memset(ips, 0, 4 * sizeof(uint32_t));
    ips[0] = (IP & 0x000000ff) >> 0;
    ips[1] = (IP & 0x0000ff00) >> 8;
    ips[2] = (IP & 0x00ff0000) >> 16;
    ips[3] = (IP & 0xff000000) >> 24;
    oss << ips[3] << "." << ips[2] << "." << ips[1] << "." << ips[0];
    return oss.str();
}

bool MapComp(const pair<int, int>& lhs, const pair<int, int>& rhs) {
    return (lhs.second < rhs.second);
}

Geography::Geography(string filename) {
    // Open File
    ifstream file(filename);
    // Check File Status
    if (!file.is_open()) {
        std::cerr << "Fail to Open File: " << filename << std::endl;
        exit(1);
    }
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
        // Check Duplicate Id and Ip
        if ((this->nodes.find(id) == this->nodes.end()) && (this->IPmap.find(IP_UINT(ip)) == this->IPmap.end())) {
            this->nodes[id] = Node(id, type, ip);
            if (ip != "NO_IP") {
                this->IPmap[IP_UINT(ip)] = id;
            }
        }
        else {
            std::cerr << "Duplicate Node with id " << id << " or ip " << ip << std::endl;
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
        // Mark Node 2 as the Neighbor of Node 1
        if (this->edges.find(id1) == this->edges.end()) {
            this->edges[id1] = {Neighbor(id2, dist)};
        }
        else {
            this->edges[id1].push_back(Neighbor(id2, dist));
        }
        // Mark Node 1 as the Neighbor of Node 2
        if (this->edges.find(id2) == this->edges.end()) {
            this->edges[id2] = {Neighbor(id1, dist)};
        }
        else {
            this->edges[id2].push_back(Neighbor(id1, dist));
        }
    }
    // Close file
    file.close();

    // std::cout << "IP Info:\n";
    // for (auto pair : this->IPmap) {
    //     std::cout << pair.first << " " << pair.second->getId() << " " << pair.second->getIp() << " " << pair.second->getType() << ":\n";
    // }
    // std::cout << "Nodes Info:\n";
    // for (auto pair : this->nodes) {
    //     std::cout << pair.first << " " << pair.second->getId() << " " << pair.second->getIp() << " " << pair.second->getType() << ":\n";
    // }
    // std::cout << "Edges Info:\n";
    // for (auto pair : this->edges) {
    //     std::cout << pair.first->getId() << " " << pair.first->getIp() << ":\n";
    //     for (Neighbor neighbor : pair.second) {
    //         std::cout << neighbor.getNptr()->getId() << " " << neighbor.getNptr()->getIp() << " " << neighbor.getDistance() << "; ";
    //     }
    //     std::cout << std::endl;
    // }
}

string Geography::findServer(string clientIP) {
    uint32_t client = IP_UINT(clientIP);
    std::cout << "Client uint32_t ip is " << client << std::endl;
    // Check ClientIP in network
    if (this->IPmap.find(client) == this->IPmap.end()) {
        std::cerr << "Fail to Find Client with IP " << clientIP << std::endl;
        return "";
    }
    // Check given IP address belongs to Client
    int origin = this->IPmap[client];
    if (this->nodes[origin].getType() != NodeType::CLIENT) {
        std::cerr << "IP " << UINT_IP(client) << " Belongs to NON-CLIENT Host" << std::endl;
        std::cerr << this->nodes[origin].getId() << " " << this->nodes[origin].getIp() << " " << this->nodes[origin].getType() << std::endl;
        return "";
    }
    // Check Cache for client
    if (this->cache.find(client) != this->cache.end()) {
        std::cout << "Client IP Found in Cache" << std::endl;
        return UINT_IP(this->cache[client]);
    }
    // Find Nearest Server from Client
    Neighbor current(origin, 0);
    map<int, int> distMap = {{origin, 0}};
    unordered_set<int> visited;
    std::cout << "Start from " << client << std::endl;
    // Traverse All Nodes
    while (!distMap.empty()) {
        int currentID = current.getId();
        // Return Current Node if Server
        if (nodes[currentID].getType() == NodeType::SERVER) {
            // Dump Cache if Full
            while (cache.size() >= CACHE_SIZE) {
                cache.erase(cache.begin());
            }
            // Add Client Server Map to Cache
            cache[client] = nodes[currentID].getIp();
            return UINT_IP(nodes[currentID].getIp());
        }
        
        // Mark Current Node Visited
        std::cout << "Traverse to " << nodes[currentID].getIp() << "with distance " << current.getDistance() << std::endl;
        visited.insert(currentID);
        // Remove Current Node from DistMap
        distMap.erase(currentID);
        // Record Distance of Unvisited Neighbors of Current Node
        for (Neighbor neighbor : this->edges[currentID]) {
            int neighborID = neighbor.getId();
            // Skip Visited Nodes and Clients
            if ((visited.find(neighborID) != visited.end()) || (nodes[neighborID].getType() == NodeType::CLIENT)) {
                continue;
            }
            // If Neighbor Not Found Yet
            if (distMap.find(neighborID) == distMap.end()) {
                distMap[neighborID] = current.getDistance() + neighbor.getDistance();
            }
            // If Neighbor Already Found
            else {
                distMap[neighborID] = min(distMap[neighborID], current.getDistance() + neighbor.getDistance());
            }
        }
        // Iterate to Nearest Neighbor
        current = Neighbor(*min_element(distMap.begin(), distMap.end(), MapComp));
    }
    // No Connected Server Found
    std::cerr << "Fail to Find Connected Server from Client with IP " << client << std::endl;
    return "";
}