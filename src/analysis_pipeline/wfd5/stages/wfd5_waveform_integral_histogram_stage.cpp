#include "analysis_pipeline/wfd5/stages/wfd5_waveform_integral_histogram_stage.h"

#include <TList.h>
#include <TH1D.h>
#include <TObject.h>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>

#include "data_products/wfd5/WaveformIntegral.hh"

using namespace dataProducts;

ClassImp(WFD5WaveformIntegralHistogramStage)

void WFD5WaveformIntegralHistogramStage::OnInit() {
    inputLabel_ = parameters_.value("input_product", "WaveformIntegralCollection");
    outputLabel_ = parameters_.value("product_name", "WaveformIntegralHistogramCollection");
    titlePrefix_ = parameters_.value("title_prefix", "Integral");
    bins_ = parameters_.value("bins", 100);

    // Determine range mode based on parameters
    bool hasRelMin = parameters_.contains("relative_min");
    bool hasRelMax = parameters_.contains("relative_max");
    bool hasDynamic = parameters_.contains("dynamic_sample_size") || parameters_.contains("dynamic_mode");
    
    if (hasDynamic) {
        rangeMode_ = RangeMode::DYNAMIC;
        dynamicSampleSize_ = parameters_.value("dynamic_sample_size", 100);
        dynamicMeanOffset_ = parameters_.value("dynamic_mean_offset", 0.0);
        dynamicSigmaMultiplier_ = parameters_.value("dynamic_sigma_multiplier", 3.0);
        
        spdlog::debug("[{}] Using dynamic range: sample_size={}, mean_offset={}, sigma_multiplier={}", 
                     Name(), dynamicSampleSize_, dynamicMeanOffset_, dynamicSigmaMultiplier_);
    } else if (hasRelMin && hasRelMax) {
        rangeMode_ = RangeMode::RELATIVE;
        relativeMin_ = parameters_.value("relative_min", -1000.0);
        relativeMax_ = parameters_.value("relative_max", 1000.0);
        
        spdlog::debug("[{}] Using relative range: min={} max={}", Name(), relativeMin_, relativeMax_);
    } else {
        rangeMode_ = RangeMode::FIXED;
        min_ = parameters_.value("min", 0.0);
        max_ = parameters_.value("max", 10000.0);
        
        spdlog::debug("[{}] Using fixed range: min={} max={}", Name(), min_, max_);
    }

    spdlog::debug("[{}] Initialized with input '{}', output '{}', bins={}", 
                 Name(), inputLabel_, outputLabel_, bins_);
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

    if (getDataProductManager()->hasProduct(outputLabel_)) {
        auto outHandle = getDataProductManager()->checkoutWrite(outputLabel_);
        TList* outputList = dynamic_cast<TList*>(outHandle->getObject());

        if (!outputList) {
            spdlog::error("[{}] Output '{}' exists but is not a TList", Name(), outputLabel_);
            return;
        }

        FillHistograms(outputList, inputList);
        spdlog::debug("[{}] Processed {} entries into histogram list '{}'", 
                     Name(), inputList->GetSize(), outputLabel_);
    } else {
        auto newList = std::make_unique<TList>();
        newList->SetOwner(kTRUE);

        auto pdp = std::make_unique<PipelineDataProduct>();
        pdp->setName(outputLabel_);
        pdp->setObject(std::move(newList));
        pdp->addTag("WFD5");
        pdp->addTag("histogram");
        pdp->addTag("histogram_list");
        pdp->addTag("built_by_wfd5_waveform_integral_histogram");
        getDataProductManager()->addOrUpdate(outputLabel_, std::move(pdp));

        auto outHandle = getDataProductManager()->checkoutWrite(outputLabel_);
        TList* outputList = dynamic_cast<TList*>(outHandle->getObject());

        if (!outputList) {
            spdlog::error("[{}] Created output '{}' is not a TList", Name(), outputLabel_);
            return;
        }

        FillHistograms(outputList, inputList);
        spdlog::debug("[{}] Processed {} entries into new histogram list '{}'", 
                     Name(), inputList->GetSize(), outputLabel_);
    }
}

void WFD5WaveformIntegralHistogramStage::FillHistograms(TList* outputList, const TList* inputList) {
    for (const TObject* obj : *inputList) {
        auto* wi = dynamic_cast<const WaveformIntegral*>(obj);
        if (!wi) continue;

        std::string key = "crate_" + std::to_string(wi->crateNum)
                        + "_amc_" + std::to_string(wi->amcNum)
                        + "_ch_" + std::to_string(wi->channelTag)
                        + "_det_" + wi->detectorSystem
                        + "_subdet_" + wi->subdetector;

        // Handle dynamic range collection
        if (rangeMode_ == RangeMode::DYNAMIC) {
            if (rangeComputed_.find(key) == rangeComputed_.end()) {
                // Still collecting samples for this key
                sampleValues_[key].push_back(wi->integral);
                
                if (sampleValues_[key].size() >= static_cast<size_t>(dynamicSampleSize_)) {
                    ComputeDynamicRange(key, sampleValues_[key]);
                    rangeComputed_[key] = true;
                    
                    spdlog::debug("[{}] Computed dynamic range for {}: [{:.2f}, {:.2f}]", 
                                 Name(), key, computedMin_[key], computedMax_[key]);
                }
            }
        }

        TH1D* hist = dynamic_cast<TH1D*>(outputList->FindObject(key.c_str()));
        if (!hist) {
            auto [histMin, histMax] = GetHistogramRange(key, wi->integral);

            std::string histTitle = titlePrefix_ + " - Crate " + std::to_string(wi->crateNum)
                                                  + ", AMC " + std::to_string(wi->amcNum)
                                                  + ", Ch " + std::to_string(wi->channelTag)
                                                  + ", Det " + wi->detectorSystem
                                                  + ", Subdet " + wi->subdetector;

            hist = new TH1D(key.c_str(), histTitle.c_str(), bins_, histMin, histMax);
            hist->SetDirectory(nullptr);
            outputList->Add(hist);
        }

        // Only fill histogram if we have a valid range (for dynamic mode)
        if (rangeMode_ != RangeMode::DYNAMIC || rangeComputed_[key]) {
            hist->Fill(wi->integral);
        }
    }
}

void WFD5WaveformIntegralHistogramStage::ComputeDynamicRange(const std::string& key, 
                                                            const std::vector<double>& samples) {
    if (samples.empty()) {
        spdlog::warn("[{}] No samples collected for key {}", Name(), key);
        computedMin_[key] = 0.0;
        computedMax_[key] = 1.0;
        return;
    }

    // Calculate mean
    double mean = std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
    
    // Calculate standard deviation
    double variance = 0.0;
    for (double value : samples) {
        variance += (value - mean) * (value - mean);
    }
    variance /= samples.size();
    double sigma = std::sqrt(variance);

    // Apply user-specified offset and multiplier
    double adjustedMean = mean + dynamicMeanOffset_;
    double range = dynamicSigmaMultiplier_ * sigma;
    
    computedMin_[key] = adjustedMean - range;
    computedMax_[key] = adjustedMean + range;
    
    // Ensure we have a valid range
    if (computedMin_[key] == computedMax_[key]) {
        computedMax_[key] = computedMin_[key] + 1.0;
    }
    
    spdlog::debug("[{}] Dynamic range calculation for {}: mean={:.2f}, sigma={:.2f}, "
                 "adjusted_mean={:.2f}, range={:.2f}", 
                 Name(), key, mean, sigma, adjustedMean, range);
}

std::pair<double, double> WFD5WaveformIntegralHistogramStage::GetHistogramRange(
    const std::string& key, double currentValue) {
    
    switch (rangeMode_) {
        case RangeMode::FIXED:
            return {min_, max_};
            
        case RangeMode::RELATIVE: {
            if (firstValueMap_.find(key) == firstValueMap_.end()) {
                firstValueMap_[key] = currentValue;
            }
            double base = firstValueMap_[key];
            double histMin = base + relativeMin_;
            double histMax = base + relativeMax_;
            if (histMin == histMax) histMax = histMin + 1.0;
            return {histMin, histMax};
        }
        
        case RangeMode::DYNAMIC: {
            if (rangeComputed_[key]) {
                return {computedMin_[key], computedMax_[key]};
            } else {
                // Use a temporary range while collecting samples
                // This could be based on the current samples or a default range
                if (!sampleValues_[key].empty()) {
                    auto minmax = std::minmax_element(sampleValues_[key].begin(), sampleValues_[key].end());
                    double tempMin = *minmax.first - std::abs(*minmax.first) * 0.1;
                    double tempMax = *minmax.second + std::abs(*minmax.second) * 0.1;
                    return {tempMin, tempMax};
                } else {
                    return {min_, max_}; // Fallback to fixed range
                }
            }
        }
    }
    
    return {min_, max_}; // Fallback
}