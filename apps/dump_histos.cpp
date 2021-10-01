#include <iostream>
#include "TFile.h"
#include "HbbJetHD/HistDumper.h"
#include "HbbJetHD/ArgParser.h"

int main(int argc, char **argv){
    
    // Parse arguments
    ArgParser arg_parser(argc, argv);
    TString ifileName  = arg_parser.ifileName;
    TString ofileName  = arg_parser.ofileName;
    TString wsName     = arg_parser.wsName;
    TString snapName   = arg_parser.snapName;
    TString region     = arg_parser.regName;
    TString obsName    = arg_parser.obsName;

    // Open input file
    TFile *ifile = TFile::Open(ifileName.Data(), "READ");

    // Extract RooWorkspace and post-fit snapshot
    RooWorkspace *ws = (RooWorkspace*)ifile->Get(wsName.Data());
    ws->getSnapshot(snapName.Data());

    // Dump histograms
    HistDumper hist_dumper(*ws, region, obsName);

    // Write histograms to file
    TFile *ofile = new TFile(ofileName, "RECREATE");
    
    hist_dumper.GetData()->Write("hdata");
    hist_dumper.GetMC()->Write("hMC");

    ofile->Close();
    delete ofile;
}