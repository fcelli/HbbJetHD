#include <iostream>
#include "TFile.h"
#include "HistDumper/HistDumper.h"
#include "ArgParser.h"

int main(int argc, char **argv){
    
    // Parse arguments
    ArgParser arg_parser(argc, argv);
    TString ifile_name  = arg_parser.get_inputfile();
    TString ofile_name  = arg_parser.get_outputfile();
    TString ws_name     = arg_parser.get_workspace();
    TString snap_name   = arg_parser.get_snapshot();
    TString region      = arg_parser.get_region();
    TString observable  = arg_parser.get_observable();

    // Open input file
    TFile *ifile = TFile::Open(ifile_name.Data(), "READ");

    // Extract RooWorkspace and post-fit snapshot
    RooWorkspace *ws = (RooWorkspace*)ifile->Get(ws_name.Data());
    ws->getSnapshot(snap_name.Data());

    HistDumper hd(*ws);
}