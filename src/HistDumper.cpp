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
        } delete fcn_iter;

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
            } delete var_iter;
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
                m_pars_QCD.push_back(constri);
                m_pars.push_back(constri);
            } delete constrIter;
        }
    } delete pdf_iter;

    // Check that QCD, ttbar and Zboson histograms were found
    if(m_hQCD == NULL || m_httbar == NULL || m_hZboson == NULL){
        std::cerr << "ERROR: could not find hQCD, httbar or hZboson in input file." << std::endl;
        exit(1);
    }

    // Retrieve correlation matrix
    m_corr = makeCorrMatrix(m_pars);

    // Retrieve correlation matrix of QCD parameters
    m_corr_QCD = makeCorrMatrix(m_pars_QCD);

    // Compute MC error band
    TGraphAsymmErrors *error_band = errorBandMC();
    
    // Create total MC histogram
    for(int i=0; i<m_histos.size(); ++i){
        if(i==0) m_hMC = (TH1F*)m_histos[i]->Clone("hMC");
        else m_hMC->Add(m_histos[i]);
    }

    // Assign error band to total MC histogram
    for(int i=1; i<=m_hMC->GetNbinsX(); ++i){
        m_hMC->SetBinError(i, error_band->GetErrorY(i-1));
    }
}

CorrMatrix HistDumper::makeCorrMatrix(std::vector<RooRealVar*> pars){
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

TGraphAsymmErrors* HistDumper::errorBandMC(){
    int hQCD_nbins   = m_hQCD->GetNbinsX();
    float hQCD_xlow  = m_hQCD->GetBinLowEdge(1);
    float hQCD_xhigh = m_hQCD->GetBinLowEdge(hQCD_nbins + 1);

    // Define QCD function
    TF1* f = new TF1("f", "[6]*TMath::Exp([0]*TMath::Power((x-140.)/70.,1)+[1]*TMath::Power((x-140.)/70.,2)+[2]*TMath::Power((x-140.)/70.,3)+[3]*TMath::Power((x-140.)/70.,4)+[4]*TMath::Power((x-140.)/70.,5)+[5]*TMath::Power((x-140.)/70.,6))", hQCD_xlow, hQCD_xhigh);

    // Set QCD function parameters
    for (int i=0; i<6; i++) {
        f->SetParameter(i, (i<=m_pars_QCD.size()-1) ? m_pars_QCD[i]->getValV() : 0);
    }
    f->SetParameter(6, 1e3);
    f->SetParameter(6, 1e3 * (m_hQCD->Integral()) / (f->Integral(hQCD_xlow, hQCD_xhigh)));

    // Retrieve all bin-by-bin variations
    TF1* f_clone = (TF1*)f->Clone("f_clone");
    std::map<TString, std::vector<std::vector<Double_t>>> par_var_bin;
    // Retrieve QCD variations
    for(int i=0; i < m_pars_QCD.size(); i++){
        TString parName = (TString)m_pars_QCD[i]->GetName();
        std::vector<std::vector<Double_t>> var_bin;
        Double_t idown = m_pars_QCD[i]->getValV() - fabs(m_pars_QCD[i]->getErrorLo());
        Double_t iup   = m_pars_QCD[i]->getValV() + fabs(m_pars_QCD[i]->getErrorHi());
        for(int bin=1; bin <= hQCD_nbins; bin++){
            float bin_low_edge  = m_hQCD->GetBinLowEdge(bin);
            float bin_high_edge = m_hQCD->GetBinLowEdge(bin) + m_hQCD->GetBinWidth(bin);
            f_clone->SetParameter(i, idown);
            Double_t down = f_clone->Integral(bin_low_edge, bin_high_edge) - m_hQCD->GetBinContent(bin);
            f_clone->SetParameter(i, iup);
            Double_t up   = f_clone->Integral(bin_low_edge, bin_high_edge) - m_hQCD->GetBinContent(bin);
            if (down<=0 && up>=0) 		var_bin.push_back({down, up});
            else if (down>=0 && up<=0) 	var_bin.push_back({up, down});
            else {
                std::cout << "WARNING: error band from parameter " << m_pars_QCD[i]->GetName() << " is one-sided in bin " << bin << std::endl;
                var_bin.push_back({down, up});
            }
        }
        par_var_bin[parName] = var_bin;
        // Reset
        f_clone = (TF1*)f->Clone("f_clone");
    }
    // Retrieve variations from Z and ttbar
    for(int i=0; i<m_pars.size(); i++){
        TString parName = (TString)m_pars[i]->GetName();
        if (!parName.Contains("mu") && !parName.Contains("yield_QCD")) continue;
        std::vector<std::vector<Double_t>> var_bin;
        // Zboson
        if (parName.Contains("Zboson")) {
            for(int bin=1; bin <= hQCD_nbins; bin++){
                Double_t down = -fabs(m_pars[i]->getErrorLo()) * m_hZboson->GetBinContent(bin);
                Double_t up   = fabs(m_pars[i]->getErrorHi()) * m_hZboson->GetBinContent(bin);
                var_bin.push_back({down, up});
            }
        par_var_bin[parName] = var_bin;
        }
        // ttbar
        else if (parName.Contains("ttbar")) {
            for(int bin=1; bin <= hQCD_nbins; bin++){
                Double_t down = -fabs(m_pars[i]->getErrorLo()) * m_httbar->GetBinContent(bin);
                Double_t up   = fabs(m_pars[i]->getErrorHi()) * m_httbar->GetBinContent(bin);
                var_bin.push_back({down, up});
            }
            par_var_bin[parName] = var_bin;
        }
        // yield_QCD
        else {
            for(int bin=1; bin <= hQCD_nbins; bin++){
                Double_t down = -fabs(m_pars[i]->getErrorLo()) * m_hQCD->GetBinContent(bin) / m_pars[i]->getValV(); 
                Double_t up   = fabs(m_pars[i]->getErrorHi()) * m_hQCD->GetBinContent(bin) / m_pars[i]->getValV();
                var_bin.push_back({down, up});
            }
            par_var_bin[parName] = var_bin;
        }
    }

    // Combine all bin-by-bin variations with cross-correlations
    Double_t x[hQCD_nbins], y[hQCD_nbins], exl[hQCD_nbins], exh[hQCD_nbins], eyl[hQCD_nbins], eyh[hQCD_nbins];
    for(int bin=1; bin <= hQCD_nbins; bin++){
        Double_t err_up(0.), err_down(0.);
        for(int i=0; i < m_pars.size(); i++){
            TString pariName = (TString)m_pars[i]->GetName();
            err_down += par_var_bin[pariName][bin-1][0]*par_var_bin[pariName][bin-1][0];
            err_up   += par_var_bin[pariName][bin-1][1]*par_var_bin[pariName][bin-1][1];
            for(int j=0; j < m_pars.size(); j++){
                if (i>=j) continue;
                TString parjName = (TString)m_pars[j]->GetName();
                err_down	+= 2*m_corr[pariName][parjName]*par_var_bin[pariName][bin-1][0]*par_var_bin[parjName][bin-1][0];
                err_up		+= 2*m_corr[pariName][parjName]*par_var_bin[pariName][bin-1][1]*par_var_bin[parjName][bin-1][1];
            }
        }
        err_down = sqrt(err_down);
        err_up 	 = sqrt(err_up);
        x[bin-1]	= m_hQCD->GetBinCenter(bin);
        exl[bin-1]	= m_hQCD->GetBinWidth(bin)/2;
        exh[bin-1]	= m_hQCD->GetBinWidth(bin)/2;
        y[bin-1]	= m_hQCD->GetBinContent(bin);
        eyl[bin-1]	= err_down;
        eyh[bin-1]	= err_up;
    }
    TGraphAsymmErrors *tgae = new TGraphAsymmErrors(hQCD_nbins, x, y, exl, exh, eyl, eyh);
    return tgae;
}