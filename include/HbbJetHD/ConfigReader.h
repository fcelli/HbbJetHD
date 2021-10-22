#ifndef CONFIGREADER_H
#define CONFIGREADER_H

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include "TEnv.h"
#include "TObjString.h"
#include "TObjArray.h"

class ConfigReader {
    public:
        ConfigReader(const TString &filename);
        TString wsName() const {return m_wsName;}
        TString snapName() const {return m_snapName;}
        TString regName() const {return m_regName;}
        TString obsName() const {return m_obsName;}
        TString dataName() const {return m_dataName;}
        TString resName() const {return m_resName;}
        bool doNPs() const {return m_doNPs;}
        std::vector<TString> bkgProcs() const {return m_bkgProcs;}
    private:
        // Workspace name
        TString m_wsName;
        // Snapshot name
        TString m_snapName;
        // Region name
        TString m_regName;
        // Observable name
        TString m_obsName;
        // RooDataSet name
        TString m_dataName;
        // RooFitResult name
        TString m_resName;
        // Flag for including NPs in the error band calculation
        bool m_doNPs;
        // Names of the background processes
        std::vector<TString> m_bkgProcs;
        std::vector<TString> splitString(TString str, char delim);
        std::map<TString, TString> readMap(TString str, char delim1=',', char delim2=':');
};

#endif