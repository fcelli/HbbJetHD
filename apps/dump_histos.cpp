#include <iostream>
#include "TFile.h"
#include "HbbJetHD/HistDumper.h"
#include "HbbJetHD/ArgParser.h"
#include "HbbJetHD/ConfigReader.h"

int main(int argc, char **argv){
    
    // Parse arguments
    ArgParser arg_parser(argc, argv);
    const TString ifileName = arg_parser.ifileName;
    const TString ofileName = arg_parser.ofileName;
    const TString cfgName   = arg_parser.cfgName;

    // Open config file
    ConfigReader config_reader(cfgName);
    const TString wsName   = config_reader.wsName();
    const TString snapName = config_reader.snapName();
    const TString regName  = config_reader.regName();
    const TString obsName  = config_reader.obsName();
    const TString dataName = config_reader.dataName();
    const TString resName  = config_reader.resName();
    const bool doNPs = config_reader.doNPs();
    const std::vector<TString> bkgProcs = config_reader.bkgProcs();

    // Open input file
    TFile *ifile = TFile::Open(ifileName.Data(), "READ");

    // Dump histograms
    HistDumper hist_dumper(ifile, wsName, snapName, regName, obsName, dataName, resName, doNPs, bkgProcs);

    // Write histograms to file
    TFile *ofile = new TFile(ofileName, "RECREATE");
    
    hist_dumper.getData()->Write("hdata");
    hist_dumper.getMC()->Write("hMC");

    ofile->Close();
    delete ifile, ofile;
}