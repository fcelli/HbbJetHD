#include "HistDumper.h"

HistDumper::HistDumper(TFile *ifile, const TString &wsName, const TString &snapName, const TString &regName, const TString &obsName){
    
    // Extract RooWorkspace and post-fit snapshot
    m_ws = (RooWorkspace*)ifile->Get(wsName.Data());
    if(m_ws == NULL){
        std::cerr << "ERROR: no workspace with name " << wsName << " found in input file." << std::endl;
        exit(1);
    }
    m_ws->getSnapshot(snapName.Data());

    // Extract dataset and data histogram
    TString datasetName = "combData";
    m_data = dynamic_cast<RooDataSet*>(m_ws->data(datasetName.Data()));
    if(m_data == NULL){
        std::cerr << "ERROR: no dataset with name " << datasetName << " found in input file." << std::endl;
        exit(1);
    }
    m_hdata = (TH1F*)m_data->createHistogram(obsName);
    m_hdata->Sumw2();
    // Enforce Gaussian errors on data points
    for(int ibin = 1; ibin <= m_hdata->GetNbinsX(); ibin ++){
        m_hdata->SetBinError(ibin, sqrt(m_hdata->GetBinContent(ibin)));
    }

    // Extract fit result
    TString fitResultName = "fitResult";
    m_fitResult = (RooFitResult*)ifile->Get(fitResultName.Data());
    if(m_fitResult == NULL){
        std::cerr << "ERROR: no fit result with name " << fitResultName << " found in input file." << std::endl;
        exit(1);
    }

    // Extract MC histograms
    RooArgSet all_pdfs  = (RooArgSet)m_ws->allPdfs();
    TIterator *pdf_iter = all_pdfs.createIterator();
    RooAbsPdf *pdfi;
	while(pdfi=(RooAbsPdf*)pdf_iter->Next()){
        TString pdfName = (TString)pdfi->GetName();
        if(!pdfName.BeginsWith("pdf__")) continue;
        if (!pdfName.Contains(regName)) continue;
        
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
        RooArgSet all_fcns = (RooArgSet)m_ws->allFunctions();
        TIterator* fcn_iter = all_fcns.createIterator();
        RooRealVar *yieldi;
        while((yieldi=(RooRealVar*)fcn_iter->Next())){
            TString yieldName = (TString)yieldi->GetName();
            if (!yieldName.BeginsWith("yield__") || !yieldName.Contains(procName) || !yieldName.Contains(regName)) continue;
            else {
                std::cout << "\tyield name:\t" << yieldi->GetName() << "\tyield value:\t" << yieldi->getVal() << std::endl;
                break;
            }
        }
        delete fcn_iter;

        // Scale histogram by corresponding yield
        h->Scale(yieldi->getVal()/h->Integral());

        m_histos.push_back(h);
        // Save QCD, ttbar and Zboson histograms
        if(pdfName.Contains("QCD")) m_hQCD = (TH1F*)h->Clone("hQCD");
        else if(pdfName.Contains("ttbar")) m_httbar = (TH1F*)h->Clone("httbar");
        else if(pdfName.Contains("Zboson")) m_hZboson = (TH1F*)h->Clone("hZboson");

        // Extract ttbar and Z mu values
        if (pdfName.Contains("ttbar") || pdfName.Contains("Zboson") || pdfName.Contains("Higgs")){
            std::cout << "Extract ttbar & Z signal strengths..." << std::endl;
            RooArgSet *allYieldVariables = (RooArgSet*)yieldi->getVariables();
            TIterator* var_iter = allYieldVariables->createIterator();
            RooRealVar* vari;
            while((vari=(RooRealVar*)var_iter->Next())) {
                if (!TString(vari->GetName()).BeginsWith("mu")) continue;
                if (pdfName.Contains("ttbar")) m_pars.push_back(vari);
                if (pdfName.Contains("Z")) m_pars.push_back(vari);
                //if (pdfName.Contains("Higgs")) 	{ mu_Higgs_values.insert({procName,vari->getVal()}); NH++; }
            }
            delete var_iter;
        }

        if (pdfName.Contains("QCD")){
            // Extract QCD yield
            TString yield_QCD_name = "yield_QCD_" + obsName(6, obsName.Length());
            m_yield_QCD = (RooRealVar*)m_ws->var(yield_QCD_name);
            if (m_yield_QCD == NULL){
                std::cerr << "ERROR: QCD yield with name " << yield_QCD_name << "not found."<<std::endl;
                exit(1);
            }
            m_pars.push_back(m_yield_QCD);

            // Extract QCD function parameters
            std::cout << "Extracting QCD function parameters..." << std::endl;
            RooArgSet* funcPars = pdfi->getParameters(*m_data);
            TIterator* constrIter = funcPars->createIterator();
            RooRealVar* constri;
            while(constri=(RooRealVar*)constrIter->Next()){
                if(constri->isConstant()) continue;
                std::cout << "\tParameter name:\t" << constri->GetName() << "\tValue:\t" << constri->getValV() << " + " << constri->getErrorHi() << " - " << constri->getErrorLo() << std::endl;
                //central_down_up_pars.push_back({constri->getValV(),constri->getValV()-fabs(constri->getErrorLo()),constri->getValV()+fabs(constri->getErrorHi())});
                m_pars_QCD.push_back(constri);
                m_pars.push_back(constri);
            }
        }
    }
    delete pdf_iter;

    // Check that QCD, ttbar and Zboson histograms were found
    if(m_hQCD == NULL || m_httbar == NULL || m_hZboson == NULL){
        std::cerr << "ERROR: could not find hQCD, httbar or hZboson in input file." << std::endl;
        exit(1);
    }

    // Retrieve correlation matrix
    m_corr = MakeCorrMatrix(m_pars);

    // Retrieve correlation matrix of QCD parameters
    m_corr_QCD = MakeCorrMatrix(m_pars_QCD);

    m_hMC = m_histos[0];
    for(int i=1; i<m_histos.size(); ++i) {
        m_hMC->Add(m_histos[i]);
    }
}

CorrMatrix HistDumper::MakeCorrMatrix(std::vector<RooRealVar*> pars){
    CorrMatrix matrix;
    for(int i=0; i < pars.size(); i++){
        for(int j=0; j < pars.size(); j++){
            TString pariName = (TString)pars[i]->GetName();
            TString parjName = (TString)pars[j]->GetName();
            matrix[pariName][parjName] = m_fitResult->correlation(pariName.Data(), parjName.Data());
        }
    }
    return matrix;
}