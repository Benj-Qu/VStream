#ifndef MIPROXY_H
#define MIPROXY_H

#include <iostream>

using namespace std;

static const int MAX_CLIENT = 100;

class MiProxy {
   public:
    void get_options(int argc, char *argv[]);
    void init();
    void run();

   private:
    bool dnsMode;
    int listenPort;
    int wwwIp;
    int dnsIp;
    int dnsPort;
    float alpha;
    string logPath;

    fd_set readfds;
    vector<int> client_socket(MAX_CLIENT, 0);
};

#endif
