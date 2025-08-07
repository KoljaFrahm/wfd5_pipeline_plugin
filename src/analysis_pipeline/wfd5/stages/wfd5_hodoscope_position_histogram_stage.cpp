#include "analysis_pipeline/wfd5/stages/wfd5_hodoscope_position_histogram_stage.h"

#include <TH2D.h>
#include <TObject.h>
#include <spdlog/spdlog.h>

ClassImp(WFD5HodoscopePositionHistogramStage)

void WFD5HodoscopePositionHistogramStage::OnInit() {
    inputLabel_ = parameters_.value("input_product", "HodoscopeEvent");
    outputLabel_ = parameters_.value("product_name", "HodoscopePositionHistogram");
    title_ = parameters_.value("title", "Hodoscope Position");
    binsX_ = parameters_.value("bins_x", 100);
    binsY_ = parameters_.value("bins_y", 100);
    xMin_ = parameters_.value("x_min", 0.0);
    xMax_ = parameters_.value("x_max", 100.0);
    yMin_ = parameters_.value("y_min", 0.0);
    yMax_ = parameters_.value("y_max", 100.0);

    spdlog::debug("[{}] Initialized with input '{}', output '{}', binsX={}, binsY={}, xRange=[{},{}], yRange=[{},{}]",
                  Name(), inputLabel_, outputLabel_, binsX_, binsY_, xMin_, xMax_, yMin_, yMax_);
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
        auto newHist = std::make_unique<TH2D>(outputLabel_.c_str(), title_.c_str(), binsX_, xMin_, xMax_, binsY_, yMin_, yMax_);
        newHist->SetDirectory(nullptr);

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
    // Only fill if max_x and max_y are valid (optional sanity check)
    if (evt->max_integral_x < 0 || evt->max_integral_y < 0) {
        spdlog::debug("[{}] Skipping fill: invalid max_integral_x={} or max_integral_y={}", 
                      Name(), evt->max_integral_x, evt->max_integral_y);
        return;
    }

    hist->Fill(evt->max_x, evt->max_y);
}
