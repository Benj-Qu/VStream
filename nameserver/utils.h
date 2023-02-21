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
    NodeType type;
    string ip;

public:
    Node(string _type, string _ip);
    NodeType getType() {return type;};
    string getIp() {return ip;};
};

class Neighbor {
private:
    Node* nptr;
    int distance;

public:
    Neighbor(Node* _nptr, int _distance) : nptr(_nptr), distance(_distance) {};
    Neighbor(pair<Node*, int> _neighbor) : nptr(_neighbor.first), distance(_neighbor.second) {};
    Node* getNptr() {return nptr;};
    int getDistance() {return distance;};
};

class Geography {
private:
    unordered_map<int, Node*> nodes;
    unordered_map<Node*, vector<Neighbor>> edges;
    unordered_map<string, Node*> IPmap;
    // Cached Server IP for each Client
    unordered_map<string, string> cache;

public:
    Geography() {};
    Geography(string filename);
    ~Geography();
    // Find Nearest Server
    string findServer(string client);
};

string selectServer(Info* info, RoundRobin* rr, Geography* geo, string client);

#endif