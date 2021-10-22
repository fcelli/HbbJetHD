#include "ConfigReader.h"

ConfigReader::ConfigReader(const TString &filename) {
    TEnv* config = new TEnv("env");
    if (config->ReadFile(filename.Data(), EEnvLevel(0)) == -1) {
        std::cout << "Could not read config file " << filename.Data() << std::endl;
        abort();
    }
    // Workspace name
    m_wsName = config->GetValue("wsName", "");
    // Snapshot name
    m_snapName = config->GetValue("snapName", "");
    // Region name
    m_regName = config->GetValue("regName", "");
    // Observable name
    m_obsName = config->GetValue("obsName", "");
    // RooDataSet name
    m_dataName = config->GetValue("dataName", "");
    // RooFitResult name
    m_resName = config->GetValue("resName", "");
    // Use NPs in error band computation
    std::istringstream(config->GetValue("doNPs", "")) >> std::boolalpha >> m_doNPs;
    // Background processes
    m_bkgProcs = splitString(config->GetValue("bkgProcs", ""), ',');
}

std::vector<TString> ConfigReader::splitString(TString str, char delim) {
    std::vector<TString> tokens;
    TObjArray *Strings=str.Tokenize(delim);
    for (std::size_t i=0; i < Strings->GetEntriesFast(); i++) {
      tokens.push_back(((TObjString*) (*Strings)[i])->GetString());
    }
    return tokens;
}

std::map<TString, TString> ConfigReader::readMap(TString str, char delim1, char delim2) {
    std::map<TString, TString> map;
    std::vector<TString> entries = splitString(str, delim1);
    for (TString e : entries) {
        std::vector<TString> elems = splitString(e, delim2);
        map.insert(std::pair<TString, TString>(elems[0], elems[1]));
    }
    return map;
}