#ifndef HISTDUMPER_H
#define HISTDUMPER_H

#include <iostream>
#include <vector>
#include "TFile.h"
#include "TH1F.h"
#include "TF1.h"
#include "TGraphAsymmErrors.h"

// RooFit
#include "RooWorkspace.h"
#include "RooRealVar.h"
#include "RooAbsPdf.h"
#include "RooDataSet.h"
#include "RooFitResult.h"
#include "RooSimultaneous.h"

// RooStats
#include "RooStats/ModelConfig.h"

typedef std::map<TString, std::map<TString, double>> CorrMatrix;

class HistDumper {
    public:
        HistDumper(TFile *ifile, const TString &wsName, const TString &snapName, const TString &region, const TString &obsName);
        std::vector<TH1F*> getHistos() const {return m_histos;}
        TH1F* getData() const {return m_hdata;}
        TH1F* getMC() const {return m_hMC;}
    private:
        RooWorkspace *m_ws;
        RooDataSet *m_data;
        RooFitResult *m_fitResult;
        RooRealVar *m_yield_QCD;
        std::vector<RooRealVar*> m_pars, m_pars_QCD;
        std::vector<TH1F*> m_histos;
        TH1F *m_hdata, *m_hMC, *m_hQCD, *m_httbar, *m_hZboson;
        CorrMatrix m_corr, m_corr_QCD;
        CorrMatrix makeCorrMatrix(std::vector<RooRealVar*>);
        TGraphAsymmErrors* errorBandMC();
};

#endif