#include "HistDumper.h"

HistDumper::HistDumper(TFile *ifile, const TString &wsName, const TString &snapName, const TString &regName, const TString &obsName, const TString &dataName, const TString &resName, const bool doNPs, const std::vector<TString> &bkgProcs) : m_wsName(wsName), m_snapName(snapName), m_regName(regName), m_obsName(obsName), m_dataName(dataName), m_resName(resName), m_doNPs(doNPs), m_bkg_procs(bkgProcs) {
    
    // Prevent ROOT from saving histograms into internal registry
    TH1::AddDirectory(false);

    // Extract RooWorkspace and load post-fit snapshot
    m_ws = (RooWorkspace*)ifile->Get(m_wsName.Data());
    if (m_ws == NULL) {
        std::cerr << "ERROR: no workspace with name " << m_wsName << " found in input file." << std::endl;
        exit(1);
    }
    m_ws->loadSnapshot(m_snapName.Data());

    // Extract dataset and data histogram
    m_data = dynamic_cast<RooDataSet*>(m_ws->data(m_dataName.Data()));
    if (m_data == NULL) {
        std::cerr << "ERROR: no dataset with name " << m_dataName << " found in input file." << std::endl;
        exit(1);
    }
    RooStats::ModelConfig *_mConfig = (RooStats::ModelConfig*)m_ws->obj("ModelConfig");
    RooSimultaneous *m_pdf = dynamic_cast<RooSimultaneous*>(_mConfig->GetPdf());
    RooAbsCategoryLValue *m_cat = const_cast<RooAbsCategoryLValue*>(&m_pdf->indexCat());
    int numChannels = m_cat->numBins(0);
    TList *m_dataList = m_data->split(*m_cat, true);
    for (std::size_t i=0; i < numChannels; i++) {
        m_cat->setBin(i);
        RooDataSet* datai = (RooDataSet*)m_dataList->At(i);
        if (!((TString)datai->GetName()).Contains(m_regName)) continue;
        m_hdata = (TH1F*)datai->createHistogram(m_obsName);
        m_hdata->Sumw2();
        // Enforce Gaussian errors on data points
        for (std::size_t ibin=1; ibin <= m_hdata->GetNbinsX(); ibin ++) {
            m_hdata->SetBinError(ibin, sqrt(m_hdata->GetBinContent(ibin)));
        }
    }

    // Extract fit result
    m_fitResult = (RooFitResult*)ifile->Get(m_resName.Data());
    if (m_fitResult == NULL) {
        std::cerr << "ERROR: no fit result with name " << m_resName << " found in input file." << std::endl;
        exit(1);
    }

    // Extract MC histograms
    RooArgSet all_pdfs  = (RooArgSet)m_ws->allPdfs();
    TIterator *pdf_iter = all_pdfs.createIterator();
    RooAbsPdf *pdfi;
	while (pdfi=(RooAbsPdf*)pdf_iter->Next()) {
        TString pdfName = (TString)pdfi->GetName();
        if (!pdfName.BeginsWith("pdf__")) continue;
        if (!pdfName.Contains(m_regName)) continue;
        
        // Determine process name
        TString procName = !(pdfName.Contains("_pt")) ? pdfName(5,TString(pdfName(5,pdfName.Length())).First("_")) : pdfName(5,TString(pdfName(5,pdfName.Length())).First("_pt")+4);
        if ( pdfName.Contains("_a1") || pdfName.Contains("_a4") || pdfName.Contains("_a5") || pdfName.Contains("_b10") || pdfName.Contains("_b4") || pdfName.Contains("_b5") ) {
            procName = pdfName( 5, (TString(pdfName(5,pdfName.Length())).First("_") + 1 + TString(pdfName(TString(pdfName(5,pdfName.Length())).First("_") + 1, pdfName.Length())).First("_") ) + 1 );
        }
        std::cout << "\tproc name:\t" << procName << std::endl;
        std::cout << "\tpdf name:\t" << pdfName << std::endl;

        // Transform pdf into histogram and set bin errors to 0
        TH1F* h = (TH1F*)pdfi->createHistogram(m_obsName);
        h->Sumw2();
        for (std::size_t ibin=1; ibin <= h->GetNbinsX(); ibin++) {
            h->SetBinError(ibin, 0);
        }

        // Find associated yield
        RooArgSet all_fcns = (RooArgSet)m_ws->allFunctions();
        TIterator* fcn_iter = all_fcns.createIterator();
        RooRealVar *yieldi;
        while (yieldi=(RooRealVar*)fcn_iter->Next()) {
            TString yieldName = (TString)yieldi->GetName();
            if (!yieldName.BeginsWith("yield__") || !yieldName.Contains(procName) || !yieldName.Contains(m_regName)) continue;
            else {
                std::cout << "\tyield name:\t" << yieldi->GetName() << "\tyield value:\t" << yieldi->getVal() << std::endl;
                break;
            }
        } delete fcn_iter;

        // Scale histogram by corresponding yield
        h->Scale(yieldi->getVal() / h->Integral());

        // Save histogram, pdf and yield
        m_histos[procName] = (TH1F*)h->Clone("h_" + procName);
        m_pdfs[procName]   = pdfi;
        m_yields[procName] = yieldi;

        // Extract signal strengths
        if (pdfName.Contains("ttbar") || pdfName.Contains("Zboson") || pdfName.Contains("Higgs")) {
            std::cout << "Extracting signal strengths..." << std::endl;
            RooArgSet *allYieldVariables = (RooArgSet*)yieldi->getVariables();
            TIterator* var_iter = allYieldVariables->createIterator();
            RooRealVar* vari;
            while (vari=(RooRealVar*)var_iter->Next()) {
                TString varName = (TString)vari->GetName();
                if (!varName.BeginsWith("mu")) continue;
                if (pdfName.Contains("ttbar")) m_pars.push_back(vari);
                if (pdfName.Contains("Z")) m_pars.push_back(vari);
            } delete var_iter;
        }

        // QCD-specific
        if (pdfName.Contains("QCD")) {
            // Extract QCD yield
            TString yield_QCD_name = "yield_QCD_" + m_obsName(6, m_obsName.Length());
            m_yield_QCD = (RooRealVar*)m_ws->var(yield_QCD_name);
            if (m_yield_QCD == NULL) {
                std::cerr << "ERROR: QCD yield with name " << yield_QCD_name << "not found."<<std::endl;
                exit(1);
            }
            m_pars.push_back(m_yield_QCD);

            // Extract QCD function parameters
            std::cout << "Extracting QCD function parameters..." << std::endl;
            RooArgSet *funcPars = pdfi->getParameters(*m_data);
            TIterator *constrIter = funcPars->createIterator();
            RooRealVar *constri;
            while (constri=(RooRealVar*)constrIter->Next()) {
                if (constri->isConstant()) continue;
                m_pars_QCD.push_back(constri);
                m_pars.push_back(constri);
            } delete constrIter;
        }
    } delete pdf_iter;

    // Extract nuisance parameters
    if (m_doNPs) {
        RooArgSet *allNPs = (RooArgSet*)_mConfig->GetNuisanceParameters();
        TIterator *np_iter = allNPs->createIterator();
        RooRealVar *npi;
        std::vector<TString> np_names;
        while (npi=(RooRealVar*)np_iter->Next()) {
            TString npName = (TString)npi->GetName();
            if (npName.Contains("Higgs") || npName.Contains("Hbb")) continue;
            if (!npName.BeginsWith("alpha_") && !npName.BeginsWith("xsec_unc_")) continue;
            if (std::find(np_names.begin(), np_names.end(), npName) != np_names.end()) {
                std::cout << "INFO: NP with name \"" << npName << "\" is a duplicate. Skipping.." << std::endl;
                continue;
            }
            else {
                np_names.push_back(npName);
                m_pars.push_back(npi);
            }
        } delete np_iter;
    }

    // Check if all bkg histograms were found
    for (TString bkg_proc : m_bkg_procs) {
        if (m_histos[bkg_proc] == NULL) {
            std::cerr << "ERROR: histogram for process \"" << bkg_proc << "\" was not found." << std::endl;
            exit(1);
        }
    }

    // Retrieve correlation matrix
    m_corr = makeCorrMatrix(m_pars);

    // Retrieve correlation matrix of QCD parameters
    m_corr_QCD = makeCorrMatrix(m_pars_QCD);

    // Compute MC error band
    TGraphAsymmErrors *error_band = errorBandMC();
    
    // Create total MC histogram
    for (std::size_t i=0; i < m_bkg_procs.size(); ++i) {
        if (i == 0) m_hMC = (TH1F*)m_histos[m_bkg_procs[i]]->Clone("hMC");
        else m_hMC->Add(m_histos[m_bkg_procs[i]]);
    }

    // Assign error band to total MC histogram
    for (std::size_t i=1; i <= m_hMC->GetNbinsX(); ++i) {
        m_hMC->SetBinError(i, error_band->GetErrorY(i-1));
    }
}

CorrMatrix HistDumper::makeCorrMatrix(std::vector<RooRealVar*> pars) {
    CorrMatrix matrix;
    for (std::size_t i=0; i < pars.size(); i++) {
        TString pariName = (TString)pars[i]->GetName();
        for (std::size_t j=0; j < pars.size(); j++) {
            TString parjName = (TString)pars[j]->GetName();
            matrix[pariName][parjName] = m_fitResult->correlation(pariName.Data(), parjName.Data());
        }
    }
    return matrix;
}

TGraphAsymmErrors* HistDumper::errorBandMC() {
    int nbins   = m_histos["QCD"]->GetNbinsX();
    float xlow  = m_histos["QCD"]->GetBinLowEdge(1);
    float xhigh = m_histos["QCD"]->GetBinLowEdge(nbins + 1);

    // Define QCD function
    TF1* f = new TF1("f", "[6]*TMath::Exp([0]*TMath::Power((x-140.)/70.,1)+[1]*TMath::Power((x-140.)/70.,2)+[2]*TMath::Power((x-140.)/70.,3)+[3]*TMath::Power((x-140.)/70.,4)+[4]*TMath::Power((x-140.)/70.,5)+[5]*TMath::Power((x-140.)/70.,6))", xlow, xhigh);

    // Set QCD function parameters
    for (int i=0; i<6; i++) {
        f->SetParameter(i, (i<=m_pars_QCD.size()-1) ? m_pars_QCD[i]->getValV() : 0);
    }
    f->SetParameter(6, 1e3);
    f->SetParameter(6, 1e3 * (m_histos["QCD"]->Integral()) / (f->Integral(xlow, xhigh)));

    // Retrieve all bin-by-bin variations
    TF1* f_clone = (TF1*)f->Clone("f_clone");
    std::map<TString, std::vector<std::vector<Double_t>>> par_var_bin;
    // Retrieve QCD variations
    for (std::size_t i=0; i < m_pars_QCD.size(); i++) {
        TString parName = (TString)m_pars_QCD[i]->GetName();
        std::vector<std::vector<Double_t>> var_bin;
        Double_t idown = m_pars_QCD[i]->getValV() - fabs(m_pars_QCD[i]->getErrorLo());
        Double_t iup   = m_pars_QCD[i]->getValV() + fabs(m_pars_QCD[i]->getErrorHi());
        for (std::size_t bin=1; bin <= nbins; bin++) {
            float bin_low_edge  = m_histos["QCD"]->GetBinLowEdge(bin);
            float bin_high_edge = m_histos["QCD"]->GetBinLowEdge(bin) + m_histos["QCD"]->GetBinWidth(bin);
            f_clone->SetParameter(i, idown);
            Double_t down = f_clone->Integral(bin_low_edge, bin_high_edge) - m_histos["QCD"]->GetBinContent(bin);
            f_clone->SetParameter(i, iup);
            Double_t up   = f_clone->Integral(bin_low_edge, bin_high_edge) - m_histos["QCD"]->GetBinContent(bin);
            if (down<=0 && up>=0) 		var_bin.push_back({down, up});
            else if (down>=0 && up<=0) 	var_bin.push_back({up, down});
            else {
                std::cout << "WARNING: error band from parameter " << parName << " is one-sided in bin " << bin << std::endl;
                var_bin.push_back({down, up});
            }
        }
        par_var_bin[parName] = var_bin;
        // Reset
        f_clone = (TF1*)f->Clone("f_clone");
    }
    // Retrieve variations from Z and ttbar signal strengths, QCD yield and nuisance parameters
    for (std::size_t i=0; i < m_pars.size(); i++) {
        TString parName = (TString)m_pars[i]->GetName();
        if (!parName.BeginsWith("mu_") && !parName.Contains("yield_QCD") && !parName.BeginsWith("alpha_") && !parName.BeginsWith("xsec_unc_")) continue;
        std::vector<std::vector<Double_t>> var_bin;
        // mu_Zboson
        if (parName.BeginsWith("mu_") && parName.Contains("Zboson")) {
            for (std::size_t bin=1; bin <= nbins; bin++) {
                Double_t down = -fabs(m_pars[i]->getErrorLo()) * m_histos["Zboson"]->GetBinContent(bin);
                Double_t up   = fabs(m_pars[i]->getErrorHi()) * m_histos["Zboson"]->GetBinContent(bin);
                var_bin.push_back({down, up});
            }
        par_var_bin[parName] = var_bin;
        }
        // mu_ttbar
        else if (parName.BeginsWith("mu_") && parName.Contains("ttbar")) {
            for (std::size_t bin=1; bin <= nbins; bin++) {
                Double_t down = -fabs(m_pars[i]->getErrorLo()) * m_histos["ttbar"]->GetBinContent(bin);
                Double_t up   = fabs(m_pars[i]->getErrorHi()) * m_histos["ttbar"]->GetBinContent(bin);
                var_bin.push_back({down, up});
            }
            par_var_bin[parName] = var_bin;
        }
        // yield_QCD
        else if (parName.Contains("yield_QCD")) {
            for (std::size_t bin=1; bin <= nbins; bin++) {
                Double_t down = -fabs(m_pars[i]->getErrorLo()) * m_histos["QCD"]->GetBinContent(bin) / m_pars[i]->getValV(); 
                Double_t up   = fabs(m_pars[i]->getErrorHi()) * m_histos["QCD"]->GetBinContent(bin) / m_pars[i]->getValV();
                var_bin.push_back({down, up});
            }
            par_var_bin[parName] = var_bin;
        }

        // Nuisance parameters
        else if (parName.BeginsWith("alpha_") || parName.BeginsWith("xsec_unc_")) {
            Double_t alpha      = m_pars[i]->getValV();
            Double_t alpha_up   = alpha + fabs(m_pars[i]->getErrorHi());
            Double_t alpha_down = alpha - fabs(m_pars[i]->getErrorLo());
            TH1F *hMCup, *hMCdown, *hMCnom;
            for (TString procName : m_bkg_procs) {
                RooAbsPdf *pdfi    = m_pdfs[procName];
                RooRealVar *yieldi = m_yields[procName];
                // Compute up variation
                m_pars[i]->setVal(alpha_up);
                TH1F* h_up = (TH1F*)pdfi->createHistogram(m_obsName.Data());
                h_up->Scale(yieldi->getVal() / h_up->Integral());
                // Compute down variation
                m_pars[i]->setVal(alpha_down);
                TH1F* h_down = (TH1F*)pdfi->createHistogram(m_obsName.Data());
                h_down->Scale(yieldi->getVal() / h_down->Integral());
                // Nominal histogram
                m_pars[i]->setVal(alpha);
                TH1F* h_nom = (TH1F*)pdfi->createHistogram(m_obsName.Data());
                h_nom->Scale(yieldi->getVal() / h_nom->Integral());

                // Create total variations
                if (procName == "QCD") {
                    hMCup   = (TH1F*)h_up->Clone("hMCup");
                    hMCdown = (TH1F*)h_down->Clone("hMCdown");
                    hMCnom  = (TH1F*)h_nom->Clone("hMCnom");
                }
                else {
                    hMCup->Add(h_up);
                    hMCdown->Add(h_down);
                    hMCnom->Add(h_nom);
                }
            }
            for (std::size_t bin=1; bin <= nbins; bin++) {
                Double_t down = hMCdown->GetBinContent(bin) - hMCnom->GetBinContent(bin);
                Double_t up   = hMCup->GetBinContent(bin) - hMCnom->GetBinContent(bin);
                if (down<=0 && up>=0) 		var_bin.push_back({down, up});
                else if (down>=0 && up<=0) 	var_bin.push_back({up, down});
                else {
                    std::cout << "WARNING: error band from nuisance parameter " << parName << " is one-sided in bin " << bin << std::endl;
                    if (down <= up) var_bin.push_back({down, up});
                    else var_bin.push_back({up, down});
                }
            }
            par_var_bin[parName] = var_bin;
        }

        else std::cout << "WARNING: skipping parameter \"" << parName <<"\".";
    }

    // Combine all bin-by-bin variations with cross-correlations
    Double_t x[nbins], y[nbins], exl[nbins], exh[nbins], eyl[nbins], eyh[nbins];
    for (std::size_t bin=1; bin <= nbins; bin++) {
        Double_t err_up(0.), err_down(0.);
        for (std::size_t i=0; i < m_pars.size(); i++) {
            TString pariName = (TString)m_pars[i]->GetName();
            err_down += par_var_bin[pariName][bin-1][0]*par_var_bin[pariName][bin-1][0];
            err_up   += par_var_bin[pariName][bin-1][1]*par_var_bin[pariName][bin-1][1];
            for (std::size_t j=0; j < m_pars.size(); j++) {
                if (i>=j) continue;
                TString parjName = (TString)m_pars[j]->GetName();
                err_down	+= 2*m_corr[pariName][parjName]*par_var_bin[pariName][bin-1][0]*par_var_bin[parjName][bin-1][0];
                err_up		+= 2*m_corr[pariName][parjName]*par_var_bin[pariName][bin-1][1]*par_var_bin[parjName][bin-1][1];
            }
        }
        err_down = sqrt(err_down);
        err_up 	 = sqrt(err_up);
        x[bin-1]	= m_histos["QCD"]->GetBinCenter(bin);
        exl[bin-1]	= m_histos["QCD"]->GetBinWidth(bin)/2;
        exh[bin-1]	= m_histos["QCD"]->GetBinWidth(bin)/2;
        y[bin-1]	= m_histos["QCD"]->GetBinContent(bin);
        eyl[bin-1]	= err_down;
        eyh[bin-1]	= err_up;
    }
    TGraphAsymmErrors *tgae = new TGraphAsymmErrors(nbins, x, y, exl, exh, eyl, eyh);
    return tgae;
}