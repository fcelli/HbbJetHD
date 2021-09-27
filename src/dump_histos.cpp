#include <iostream>
#include "TFile.h"
#include "HistDumper/HistDumper.h"

int main(int argc, char **argv){
    TFile *ifile = TFile::Open("/home/celli/hbbjet/xmlfit_boostedhbb/quickFit/output/SR_data_paper_items_210603_minos0.root", "READ");
    RooWorkspace *ws = (RooWorkspace*)ifile->Get("combWS");
    ws->getSnapshot("quickfit");
    HistDumper hd(*ws);
}