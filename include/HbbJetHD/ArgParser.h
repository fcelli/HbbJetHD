#ifndef ARGPARSER_H
#define ARGPARSER_H

#include <iostream>
#include "TString.h"

struct ArgParser {
    ArgParser(int argc, char **argv);
    void usage() const;
    TString ifileName;
    TString ofileName;
    TString wsName;
    TString snapName;
    TString regName;
    TString obsName;
};

#endif