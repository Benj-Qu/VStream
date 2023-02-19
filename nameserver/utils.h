#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <string>
#include <vector>
#include <iostream>

#include "DNSHeader.h"
#include "DNSQuestion.h"
#include "DNSRecord.h"

using std::string;

enum Mode {RR, GEO};

class Info {
public:
    Mode mode;
    int port;
    string servers;
    string log;

    Info(int argc, char** argv);

private:
    void usage();
};

#endif