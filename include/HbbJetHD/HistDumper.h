#ifndef HISTDUMPER_H
#define HISTDUMPER_H

#include <iostream>
#include <vector>
#include "TFile.h"
#include "TH1F.h"

// RooFit
#include "RooWorkspace.h"
#include "RooRealVar.h"
#include "RooAbsPdf.h"
#include "RooDataSet.h"
#include "RooFitResult.h"

typedef std::map<TString, std::map<TString, double>> CorrMatrix;

class HistDumper {
    public:
        HistDumper(TFile *ifile, const TString &wsName, const TString &snapName, const TString &region, const TString &obsName);
        std::vector<TH1F*> get_histos() const {return m_histos;}
        TH1F* GetData() const {return m_hdata;}
        TH1F* GetMC() const {return m_hMC;}
    private:
        RooWorkspace *m_ws;
        RooDataSet *m_data;
        RooFitResult *m_fitResult;
        RooRealVar *m_yield_QCD;
        std::vector<RooRealVar*> m_pars, m_pars_QCD;
        CorrMatrix m_corr, m_corr_QCD;
        std::vector<TH1F*> m_histos;
        TH1F *m_hdata;
        TH1F *m_hMC;
        TH1F *m_hQCD, *m_httbar, *m_hZboson;
        CorrMatrix MakeCorrMatrix(std::vector<RooRealVar*>);
};

#endif