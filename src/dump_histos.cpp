#include <iostream>
#include "TFile.h"
#include "HistDumper/HistDumper.h"
#include "ArgParser.h"

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
    for(TH1F *h : hist_dumper.get_histos()){
        h->Write();
    }
    ofile->Close();
    delete ofile;
}