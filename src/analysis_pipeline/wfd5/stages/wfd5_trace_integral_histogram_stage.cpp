#include "analysis_pipeline/wfd5/stages/wfd5_trace_integral_histogram_stage.h"

#include <TList.h>
#include <TH1D.h>
#include <TObject.h>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <string>

#include "analysis_pipeline/wfd5/data_products/wfd5_trace_integral.h"

using namespace dataProducts;

ClassImp(WFD5TraceIntegralHistogramStage)

void WFD5TraceIntegralHistogramStage::OnInit() {
    inputLabel_ = parameters_.value("input_product", "WFD5TraceIntegralCollection");
    outputLabel_ = parameters_.value("product_name", "WFD5TraceIntegralHistogramCollection");
    titlePrefix_ = parameters_.value("title_prefix", "Integral");
    bins_ = parameters_.value("bins", 100);

    bool hasRelMin = parameters_.contains("relative_min");
    bool hasRelMax = parameters_.contains("relative_max");
    useRelativeRange_ = hasRelMin && hasRelMax;

    if (useRelativeRange_) {
        relativeMin_ = parameters_.value("relative_min", -1000.0);
        relativeMax_ = parameters_.value("relative_max", 1000.0);
        spdlog::debug("[{}] Using relative range: min={} max={}", Name(), relativeMin_, relativeMax_);
    } else {
        min_ = parameters_.value("min", 0.0);
        max_ = parameters_.value("max", 10000.0);
        spdlog::debug("[{}] Using fixed range: min={} max={}", Name(), min_, max_);
    }

    spdlog::debug("[{}] Initialized with input '{}', output '{}', bins={}", Name(), inputLabel_, outputLabel_, bins_);
}

void WFD5TraceIntegralHistogramStage::Process() {
    if (!getDataProductManager()->hasProduct(inputLabel_)) {
        spdlog::warn("[{}] Input '{}' not found", Name(), inputLabel_);
        return;
    }

    auto inputHandle = getDataProductManager()->checkoutRead(inputLabel_);
    const auto* inputList = dynamic_cast<const TList*>(inputHandle->getObject());

    if (!inputList) {
        spdlog::error("[{}] Input '{}' is not a TList", Name(), inputLabel_);
        return;
    }

    if (getDataProductManager()->hasProduct(outputLabel_)) {
        auto outHandle = getDataProductManager()->checkoutWrite(outputLabel_);
        TList* outputList = dynamic_cast<TList*>(outHandle->getObject());

        if (!outputList) {
            spdlog::error("[{}] Output '{}' exists but is not a TList", Name(), outputLabel_);
            return;
        }

        FillHistograms(outputList, inputList);
        spdlog::debug("[{}] Processed {} entries into histogram list '{}'", Name(), inputList->GetSize(), outputLabel_);
    } else {
        auto newList = std::make_unique<TList>();
        newList->SetOwner(kTRUE);

        auto pdp = std::make_unique<PipelineDataProduct>();
        pdp->setName(outputLabel_);
        pdp->setObject(std::move(newList));
        pdp->addTag("WFD5");
        pdp->addTag("histogram");
        pdp->addTag("histogram_list");
        pdp->addTag("built_by_wfd5_trace_integral_histogram");
        getDataProductManager()->addOrUpdate(outputLabel_, std::move(pdp));

        auto outHandle = getDataProductManager()->checkoutWrite(outputLabel_);
        TList* outputList = dynamic_cast<TList*>(outHandle->getObject());

        if (!outputList) {
            spdlog::error("[{}] Created output '{}' is not a TList", Name(), outputLabel_);
            return;
        }

        FillHistograms(outputList, inputList);
        spdlog::debug("[{}] Processed {} entries into new histogram list '{}'", Name(), inputList->GetSize(), outputLabel_);
    }
}

void WFD5TraceIntegralHistogramStage::FillHistograms(TList* outputList, const TList* inputList) {
    std::unordered_map<std::string, double> firstValueMap;

    for (const TObject* obj : *inputList) {
        auto* ci = dynamic_cast<const WFD5TraceIntegral*>(obj);
        if (!ci) continue;

        std::string key = "crate_" + std::to_string(ci->crateNum)
                        + "_amc_" + std::to_string(ci->amcNum)
                        + "_ch_" + std::to_string(ci->channelNum);

        TH1D* hist = dynamic_cast<TH1D*>(outputList->FindObject(key.c_str()));
        if (!hist) {
            double histMin, histMax;

            if (useRelativeRange_) {
                if (firstValueMap.find(key) == firstValueMap.end())
                    firstValueMap[key] = ci->integralValue;

                double base = firstValueMap[key];
                histMin = base + relativeMin_;
                histMax = base + relativeMax_;
                if (histMin == histMax) histMax = histMin + 1.0;
            } else {
                histMin = min_;
                histMax = max_;
            }

            std::string histTitle = titlePrefix_ + " - Crate " + std::to_string(ci->crateNum)
                                                  + ", AMC " + std::to_string(ci->amcNum)
                                                  + ", Ch " + std::to_string(ci->channelNum);

            hist = new TH1D(key.c_str(), histTitle.c_str(), bins_, histMin, histMax);
            hist->SetDirectory(nullptr);
            outputList->Add(hist);
        }

        hist->Fill(ci->integralValue);
    }
}

