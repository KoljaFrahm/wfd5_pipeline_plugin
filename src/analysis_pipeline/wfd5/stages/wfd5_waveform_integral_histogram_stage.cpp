#include "analysis_pipeline/wfd5/stages/wfd5_waveform_integral_histogram_stage.h"

#include <TList.h>
#include <TH1D.h>
#include <TObject.h>
#include <spdlog/spdlog.h>
#include <string>

#include "data_products/wfd5/WaveformIntegral.hh"
#include "analysis_pipeline/wfd5/data_products/wfd5_waveform_integral_presamples.h"

using namespace dataProducts;

ClassImp(WFD5WaveformIntegralHistogramStage)

void WFD5WaveformIntegralHistogramStage::OnInit() {
    inputLabel_ = parameters_.value("input_product", "WaveformIntegralCollection");
    outputLabel_ = parameters_.value("product_name", "WaveformIntegralHistogramCollection");
    presampleLabel_ = outputLabel_ + "_Presamples";
    titlePrefix_ = parameters_.value("title_prefix", "Integral");
    bins_ = parameters_.value("bins", 100);

    // dynamic mode
    if (parameters_.contains("dynamic_sample_size")) {
        useDynamic_ = true;
        dynamicSampleSize_ = parameters_.value("dynamic_sample_size", 100);
        dynamicMeanOffset_ = parameters_.value("dynamic_mean_offset", 0.0);
        dynamicSigmaMultiplier_ = parameters_.value("dynamic_sigma_multiplier", 3.0);
        spdlog::debug("[{}] Using dynamic range mode: samples={} offset={} sigmaMult={}",
                      Name(), dynamicSampleSize_, dynamicMeanOffset_, dynamicSigmaMultiplier_);
    } else {
        // legacy mode
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
    }

    spdlog::debug("[{}] Initialized with input '{}', output '{}'", Name(), inputLabel_, outputLabel_);
}

void WFD5WaveformIntegralHistogramStage::Process() {
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

    // get/create histogram list
    TList* histList = nullptr;
    if (getDataProductManager()->hasProduct(outputLabel_)) {
        auto outHandle = getDataProductManager()->checkoutWrite(outputLabel_);
        histList = dynamic_cast<TList*>(outHandle->getObject());
    } else {
        auto newList = std::make_unique<TList>();
        newList->SetOwner(kTRUE);
        auto pdp = std::make_unique<PipelineDataProduct>();
        pdp->setName(outputLabel_);
        pdp->setObject(std::move(newList));
        pdp->addTag("WFD5");
        pdp->addTag("histogram_list");
        getDataProductManager()->addOrUpdate(outputLabel_, std::move(pdp));
        auto outHandle = getDataProductManager()->checkoutWrite(outputLabel_);
        histList = dynamic_cast<TList*>(outHandle->getObject());
    }
    if (!histList) {
        spdlog::error("[{}] Failed to get histogram list '{}'", Name(), outputLabel_);
        return;
    }

    // get/create presample list (only if dynamic)
    TList* presampleList = nullptr;
    if (useDynamic_) {
        if (getDataProductManager()->hasProduct(presampleLabel_)) {
            auto preHandle = getDataProductManager()->checkoutWrite(presampleLabel_);
            presampleList = dynamic_cast<TList*>(preHandle->getObject());
        } else {
            auto newPre = std::make_unique<TList>();
            newPre->SetOwner(kTRUE);
            auto pdpPre = std::make_unique<PipelineDataProduct>();
            pdpPre->setName(presampleLabel_);
            pdpPre->setObject(std::move(newPre));
            pdpPre->addTag("WFD5");
            pdpPre->addTag("presample_list");
            getDataProductManager()->addOrUpdate(presampleLabel_, std::move(pdpPre));
            auto preHandle = getDataProductManager()->checkoutWrite(presampleLabel_);
            presampleList = dynamic_cast<TList*>(preHandle->getObject());
        }
    }

    FillHistograms(histList, presampleList, inputList);
}

void WFD5WaveformIntegralHistogramStage::FillHistograms(TList* histList, TList* presampleList, const TList* inputList) {
    for (const TObject* obj : *inputList) {
        auto* wi = dynamic_cast<const WaveformIntegral*>(obj);
        if (!wi) continue;

        std::string key = "crate_" + std::to_string(wi->crateNum)
                        + "_amc_" + std::to_string(wi->amcNum)
                        + "_ch_" + std::to_string(wi->channelTag)
                        + "_det_" + wi->detectorSystem
                        + "_subdet_" + wi->subdetector;

        // try to find histogram
        TH1D* hist = dynamic_cast<TH1D*>(histList->FindObject(key.c_str()));

        if (!hist) {
            if (useDynamic_) {
                // find/create presample
                auto* pres = dynamic_cast<WFD5WaveformIntegralPresamples*>(presampleList->FindObject(key.c_str()));
                if (!pres) {
                    pres = new WFD5WaveformIntegralPresamples(key.c_str(), dynamicSampleSize_);
                    presampleList->Add(pres);
                }
                pres->AddSample(wi->integral);

                if (!pres->IsFull()) {
                    // not ready yet
                    continue;
                }

                // compute dynamic range
                double mean = pres->Mean() + dynamicMeanOffset_;
                double sigma = pres->Sigma();
                double histMin = mean - dynamicSigmaMultiplier_ * sigma;
                double histMax = mean + dynamicSigmaMultiplier_ * sigma;
                if (histMin == histMax) histMax = histMin + 1.0;

                std::string histTitle = titlePrefix_ + " - Crate " + std::to_string(wi->crateNum)
                                      + ", AMC " + std::to_string(wi->amcNum)
                                      + ", Ch " + std::to_string(wi->channelTag)
                                      + ", Det " + wi->detectorSystem
                                      + ", Subdet " + wi->subdetector;

                hist = new TH1D(key.c_str(), histTitle.c_str(), bins_, histMin, histMax);
                hist->SetDirectory(nullptr);
                histList->Add(hist);
            } else {
                // legacy fixed/relative
                double histMin, histMax;
                if (useRelativeRange_) {
                    histMin = wi->integral + relativeMin_;
                    histMax = wi->integral + relativeMax_;
                    if (histMin == histMax) histMax = histMin + 1.0;
                } else {
                    histMin = min_;
                    histMax = max_;
                }

                std::string histTitle = titlePrefix_ + " - Crate " + std::to_string(wi->crateNum)
                                      + ", AMC " + std::to_string(wi->amcNum)
                                      + ", Ch " + std::to_string(wi->channelTag)
                                      + ", Det " + wi->detectorSystem
                                      + ", Subdet " + wi->subdetector;

                hist = new TH1D(key.c_str(), histTitle.c_str(), bins_, histMin, histMax);
                hist->SetDirectory(nullptr);
                histList->Add(hist);
            }
        }

        if (hist) hist->Fill(wi->integral);
    }
}
