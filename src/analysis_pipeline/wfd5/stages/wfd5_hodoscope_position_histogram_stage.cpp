// wfd5_hodoscope_position_histogram_stage.cpp
#include "analysis_pipeline/wfd5/stages/wfd5_hodoscope_position_histogram_stage.h"

#include <TH2D.h>
#include <TObject.h>
#include <spdlog/spdlog.h>
#include <cmath>      // for std::round
#include <vector>

ClassImp(WFD5HodoscopePositionHistogramStage)

void WFD5HodoscopePositionHistogramStage::OnInit() {
    inputLabel_ = parameters_.value("input_product", "HodoscopeEvent");
    outputLabel_ = parameters_.value("product_name", "HodoscopePositionHistogram");
    title_ = parameters_.value("title", "Hodoscope Position");

    double xMinRaw = parameters_.value("x_min", 0.0);
    double xMaxRaw = parameters_.value("x_max", 100.0);
    double yMinRaw = parameters_.value("y_min", 0.0);
    double yMaxRaw = parameters_.value("y_max", 100.0);

    xMinInt_ = static_cast<int>(std::round(xMinRaw));
    xMaxInt_ = static_cast<int>(std::round(xMaxRaw));
    yMinInt_ = static_cast<int>(std::round(yMinRaw));
    yMaxInt_ = static_cast<int>(std::round(yMaxRaw));

    binsX_ = (xMaxInt_ - xMinInt_);
    binsY_ = (yMaxInt_ - yMinInt_);

    if (binsX_ <= 0 || binsY_ <= 0) {
        spdlog::error("[{}] Invalid bin range: binsX={}, binsY={}, xRange=[{},{}], yRange=[{},{}]",
                      Name(), binsX_, binsY_, xMinInt_, xMaxInt_, yMinInt_, yMaxInt_);
    }

    spdlog::debug("[{}] Initialized with input '{}', output '{}', binsX={}, binsY={}, xRange=[{},{}], yRange=[{},{}]",
                  Name(), inputLabel_, outputLabel_, binsX_, binsY_, xMinInt_, xMaxInt_, yMinInt_, yMaxInt_);
}

void WFD5HodoscopePositionHistogramStage::BuildIntegerBinEdges(std::vector<double>& edges, int minVal, int maxVal) {
    edges.clear();
    int nEdges = (maxVal - minVal) + 1;
    edges.reserve(nEdges);
    for (int i = 0; i <= nEdges; ++i) {
        edges.push_back(minVal + i);
    }
}

void WFD5HodoscopePositionHistogramStage::Process() {
    if (!getDataProductManager()->hasProduct(inputLabel_)) {
        spdlog::warn("[{}] Input '{}' not found", Name(), inputLabel_);
        return;
    }

    auto inputHandle = getDataProductManager()->checkoutRead(inputLabel_);
    const auto* evt = dynamic_cast<const HodoscopeEvent*>(inputHandle->getObject());
    if (!evt) {
        spdlog::error("[{}] Input '{}' is not a HodoscopeEvent", Name(), inputLabel_);
        return;
    }

    TH2D* hist = nullptr;

    if (getDataProductManager()->hasProduct(outputLabel_)) {
        auto outHandle = getDataProductManager()->checkoutWrite(outputLabel_);
        hist = dynamic_cast<TH2D*>(outHandle->getObject());
        if (!hist) {
            spdlog::error("[{}] Output '{}' exists but is not a TH2D", Name(), outputLabel_);
            return;
        }
    } else {
        std::vector<double> xEdges, yEdges;
        BuildIntegerBinEdges(xEdges, xMinInt_, xMaxInt_);
        BuildIntegerBinEdges(yEdges, yMinInt_, yMaxInt_);

        auto rawHist = new TH2D(outputLabel_.c_str(), title_.c_str(),
                                binsX_, xEdges.data(),
                                binsY_, yEdges.data());
        rawHist->SetDirectory(nullptr);
        auto newHist = std::unique_ptr<TH2D>(rawHist);

        auto pdp = std::make_unique<PipelineDataProduct>();
        pdp->setName(outputLabel_);
        pdp->setObject(std::move(newHist));
        pdp->addTag("WFD5");
        pdp->addTag("histogram");
        pdp->addTag("TH2D");
        pdp->addTag("hodoscope_position");
        getDataProductManager()->addOrUpdate(outputLabel_, std::move(pdp));

        auto outHandle = getDataProductManager()->checkoutWrite(outputLabel_);
        hist = dynamic_cast<TH2D*>(outHandle->getObject());
        if (!hist) {
            spdlog::error("[{}] Created output '{}' is not a TH2D", Name(), outputLabel_);
            return;
        }
    }

    FillHistogram(hist, evt);

    spdlog::debug("[{}] Filled histogram '{}' with event max_x={}, max_y={}", Name(), outputLabel_, evt->max_x, evt->max_y);
}

void WFD5HodoscopePositionHistogramStage::FillHistogram(TH2D* hist, const HodoscopeEvent* evt) {
    // if (evt->max_integral_x < 5000 || evt->max_integral_y > -5000) {
    //     spdlog::debug("[{}] Skipping fill: invalid max_integral_x={} or max_integral_y={}, there's probably just noise",
    //                   Name(), evt->max_integral_x, evt->max_integral_y);
    //     return;
    // }

    if (evt->max_peak_to_peak_x < 100 || evt->max_peak_to_peak_y < 100) {
        spdlog::debug("[{}] Skipping fill: invalid max_peak_to_peak_x={} or max_peak_to_peak_y={}, there's probably just noise",
                      Name(), evt->max_peak_to_peak_x, evt->max_peak_to_peak_y);
        return;
    }

    hist->Fill(evt->max_x, evt->max_y);
}
