#ifndef ARGPARSER_H
#define ARGPARSER_H

#include <iostream>
#include "TString.h"

class ArgParser {
    public:
        ArgParser(int argc, char **argv);
        TString get_inputfile() const {return m_inputfile;}
        TString get_outputfile() const {return m_outputfile;}
        TString get_workspace() const {return m_workspace;}
        TString get_snapshot() const {return m_snapshot;}
        TString get_region() const {return m_region;}
        TString get_observable() const {return m_observable;}
    private:
        TString m_inputfile;
        TString m_outputfile;
        TString m_workspace;
        TString m_snapshot;
        TString m_region;
        TString m_observable;
        void usage() const;
};

#endif