#ifndef HISTDUMPER_H
#define HISTDUMPER_H

#include <iostream>
#include <vector>
#include "TH1F.h"

// RooFit
#include "RooWorkspace.h"
#include "RooRealVar.h"
#include "RooAbsPdf.h"
#include "RooDataSet.h"

class HistDumper {
    public:
        HistDumper(const RooWorkspace &ws, const TString &region, const TString &obsName);
        std::vector<TH1F*> get_histos() const {return m_histos;}
        TH1F* GetData() const {return m_hdata;}
        TH1F* GetMC() const {return m_hMC;}
    private:
        std::vector<TH1F*> m_histos;
        TH1F *m_hdata;
        TH1F *m_hMC;
};

#endif