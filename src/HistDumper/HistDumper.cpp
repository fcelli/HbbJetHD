#include "HistDumper.h"

HistDumper::HistDumper(const RooWorkspace &ws, const TString &region, const TString &obsName){
    
    // Extract data histogram
    RooDataSet* dataset = dynamic_cast<RooDataSet*>(ws.data("combData"));
    m_hdata = (TH1F*)dataset->createHistogram(obsName);
    m_hdata->Sumw2();
    // Enforce Gaussian errors on data points
    for(int ibin = 1; ibin <= m_hdata->GetNbinsX(); ibin ++){
        m_hdata->SetBinError(ibin, sqrt(m_hdata->GetBinContent(ibin)));
    }

    // Extract MC histograms
    RooArgSet all_pdfs  = (RooArgSet)ws.allPdfs();
    TIterator *pdf_iter = all_pdfs.createIterator();
    RooAbsPdf *pdfi;
	while(pdfi=(RooAbsPdf*)pdf_iter->Next()){
        TString pdfName = (TString)pdfi->GetName();
        if(!pdfName.BeginsWith("pdf__")) continue;
        if (!pdfName.Contains(region)) continue;
        
        // Determine process name
        TString procName = !(pdfName.Contains("_pt")) ? pdfName(5,TString(pdfName(5,pdfName.Length())).First("_")) : pdfName(5,TString(pdfName(5,pdfName.Length())).First("_pt")+4);
        if ( pdfName.Contains("_a1") || pdfName.Contains("_a4") || pdfName.Contains("_a5") || pdfName.Contains("_b10") || pdfName.Contains("_b4") || pdfName.Contains("_b5") ) {
            procName = pdfName( 5, (TString(pdfName(5,pdfName.Length())).First("_") + 1 + TString(pdfName(TString(pdfName(5,pdfName.Length())).First("_") + 1, pdfName.Length())).First("_") ) + 1 );
        }
        std::cout << "\tproc name:\t" << procName << std::endl;
        std::cout << "\tpdf name:\t" << pdfName << std::endl;

        // Transform pdf into histogram and set bin errors to 0
        TH1F* h = (TH1F*)pdfi->createHistogram(obsName);
        h->Sumw2();
        for (unsigned int ibin=1; ibin<=h->GetNbinsX(); ibin++) {
            h->SetBinError(ibin, 0);
        }

        // Find associated yield
        RooArgSet all_fcns = (RooArgSet)ws.allFunctions();
        TIterator* fcn_iter = all_fcns.createIterator();
        RooRealVar *yieldi;
        while((yieldi=(RooRealVar*)fcn_iter->Next())){
            TString yieldName = (TString)yieldi->GetName();
            if (!yieldName.BeginsWith("yield__") || !yieldName.Contains(procName) || !yieldName.Contains(region)) continue;
            else {
                std::cout << "\tyield name:\t" << yieldi->GetName() << "\tyield value:\t" << yieldi->getVal() << std::endl;
                break;
            }
        }
        delete fcn_iter;

        // Scale histogram by corresponding yield
        h->Scale(yieldi->getVal()/h->Integral());

        m_histos.push_back(h);
    }
    delete pdf_iter;
}