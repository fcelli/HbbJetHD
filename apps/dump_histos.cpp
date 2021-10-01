#include <iostream>
#include "TFile.h"
#include "HbbJetHD/HistDumper.h"
#include "HbbJetHD/ArgParser.h"

int main(int argc, char **argv){
    
    // Parse arguments
    ArgParser arg_parser(argc, argv);
    TString ifileName  = arg_parser.get_inputfile();
    TString ofileName  = arg_parser.get_outputfile();
    TString wsName     = arg_parser.get_workspace();
    TString snapName   = arg_parser.get_snapshot();
    TString region     = arg_parser.get_region();
    TString obsName    = arg_parser.get_observable();

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