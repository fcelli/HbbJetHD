#ifndef HISTDUMPER_H
#define HISTDUMPER_H

#include <iostream>
#include <vector>
#include "TH1F.h"

// RooFit
#include "RooWorkspace.h"
#include "RooRealVar.h"
#include "RooAbsPdf.h"

class HistDumper {
    public:
        HistDumper(const RooWorkspace &ws, const TString &region, const TString &obsName);
        std::vector<TH1F*> get_histos() const {return m_histos;}
    private:
        std::vector<TH1F*> m_histos;
};

#endif