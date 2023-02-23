#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include <map>
#include <queue>
#include <string>
#include <vector>
#include <climits>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

#include "DNSHeader.h"
#include "DNSQuestion.h"
#include "DNSRecord.h"

using std::min;
using std::map;
using std::pair;
using std::string;
using std::vector;
using std::ifstream;
using std::ofstream;
using std::unordered_set;
using std::unordered_map;

// Info

enum Mode {RR, GEO};

class Info {
private:
    Mode mode;
    int port;
    string servers;
    string log;
    void usage();

public:
    Info(int argc, char** argv);
    Mode getMode() {return mode;};
    int getPort() {return port;};
    string getServers() {return servers;};
    string getLog() {return log;};
};


// Round Robin

class RoundRobin {
private:
    vector<string> hosts;
    int index;

public:
    RoundRobin() : index(0) {};
    RoundRobin(string filename);
    // Get Next Server
    string next();
};

// Geography

enum NodeType {CLIENT, SWITCH, SERVER};

class Node {
private:
    int id; 
    NodeType type;
    uint32_t ip;

public:
    Node() : id(0), type(NodeType::CLIENT), ip(0) {};
    Node(int _id, string _type, string _ip);
    int getId() { return id;};
    NodeType getType() {return type;};
    uint32_t getIp() {return ip;};
};

class Neighbor {
private:
    int id;
    int distance;

public:
    Neighbor(int _id, int _distance) : id(_id), distance(_distance) {};
    Neighbor(pair<int, int> _neighbor) : id(_neighbor.first), distance(_neighbor.second) {};
    int getId() {return id;};
    int getDistance() {return distance;};
};

class Geography {
private:
    unordered_map<int, Node> nodes;
    unordered_map<int, vector<Neighbor>> edges;
    unordered_map<uint32_t, int> IPmap;
    // Cached Server IP for each Client
    unordered_map<uint32_t, uint32_t> cache;

public:
    Geography() {};
    Geography(string filename);
    // Find Nearest Server
    string findServer(string client);
};

uint32_t IP_UINT(string IP);
string UINT_IP(uint32_t IP);
bool MapComp(const pair<int, int>& lhs, const pair<int, int>& rhs);
string selectServer(Info* info, RoundRobin* rr, Geography* geo, string clientIP);

#endif