#ifndef HISTDUMPER_H
#define HISTDUMPER_H

#include <iostream>
#include "TH1F.h"

// RooFit
#include "RooWorkspace.h"
#include "RooRealVar.h"
#include "RooAbsPdf.h"

class HistDumper {
    public:
        HistDumper(const RooWorkspace &ws);
    private:
};

#endif