#include "analysis_pipeline/wfd5/stages/wfd5_waveform_integral_histogram_stage.h"

#include <TList.h>
#include <TH1D.h>
#include <TObject.h>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <string>
#include <cmath>
#include <algorithm>

#include "data_products/wfd5/WaveformIntegral.hh"

using namespace dataProducts;

ClassImp(WFD5WaveformIntegralHistogramStage)
ClassImp(DynamicSampleData)
ClassImp(DynamicSampleDataMap)

void WFD5WaveformIntegralHistogramStage::OnInit() {
    inputLabel_ = parameters_.value("input_product", "WaveformIntegralCollection");
    outputLabel_ = parameters_.value("product_name", "WaveformIntegralHistogramCollection");
    titlePrefix_ = parameters_.value("title_prefix", "Integral");
    bins_ = parameters_.value("bins", 100);

    // Check for dynamic range parameters
    bool hasDynamicSampleSize = parameters_.contains("dynamic_sample_size");
    useDynamicRange_ = hasDynamicSampleSize;

    if (useDynamicRange_) {
        dynamicSampleSize_ = parameters_.value("dynamic_sample_size", 100);
        dynamicMeanOffset_ = parameters_.value("dynamic_mean_offset", 0.0);
        dynamicSigmaMultiplier_ = parameters_.value("dynamic_sigma_multiplier", 3.0);
        spdlog::debug("[{}] Using dynamic range: sample_size={}, mean_offset={}, sigma_multiplier={}", 
                     Name(), dynamicSampleSize_, dynamicMeanOffset_, dynamicSigmaMultiplier_);
    } else {
        // Existing relative/fixed range logic
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

    spdlog::debug("[{}] Initialized with input '{}', output '{}', bins={}", Name(), inputLabel_, outputLabel_, bins_);
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

    // Create or get the output list
    TList* outputList = nullptr;
    DynamicSampleDataMap* sampleDataMap = nullptr;
    
    if (getDataProductManager()->hasProduct(outputLabel_)) {
        auto outHandle = getDataProductManager()->checkoutWrite(outputLabel_);
        outputList = dynamic_cast<TList*>(outHandle->getObject());

        if (!outputList) {
            spdlog::error("[{}] Output '{}' exists but is not a TList", Name(), outputLabel_);
            return;
        }

        // Get the dynamic sample data if using dynamic range
        if (useDynamicRange_) {
            std::string sampleDataKey = outputLabel_ + "_SampleData";
            if (getDataProductManager()->hasProduct(sampleDataKey)) {
                auto sampleHandle = getDataProductManager()->checkoutWrite(sampleDataKey);
                sampleDataMap = dynamic_cast<DynamicSampleDataMap*>(sampleHandle->getObject());
            }
        }
    } else {
        // Create new output list
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
        outputList = dynamic_cast<TList*>(outHandle->getObject());

        if (!outputList) {
            spdlog::error("[{}] Created output '{}' is not a TList", Name(), outputLabel_);
            return;
        }

        // Create dynamic sample data storage if needed
        if (useDynamicRange_) {
            std::string sampleDataKey = outputLabel_ + "_SampleData";
            auto sampleData = std::make_unique<DynamicSampleDataMap>();
            sampleDataMap = sampleData.get();

            auto samplePdp = std::make_unique<PipelineDataProduct>();
            samplePdp->setName(sampleDataKey);
            samplePdp->setObject(std::move(sampleData));
            samplePdp->addTag("WFD5");
            samplePdp->addTag("dynamic_sample_data");
            samplePdp->addTag("persistent");  // Persist across runs
            getDataProductManager()->addOrUpdate(sampleDataKey, std::move(samplePdp));
        }
    }

    FillHistograms(outputList, inputList);
    spdlog::debug("[{}] Processed {} entries into histogram list '{}'", Name(), inputList->GetSize(), outputLabel_);
}

void WFD5WaveformIntegralHistogramStage::FillHistograms(TList* outputList, const TList* inputList) {
    std::unordered_map<std::string, double> firstValueMap;
    DynamicSampleDataMap* sampleDataMap = nullptr;

    // Get dynamic sample data if using dynamic range
    if (useDynamicRange_) {
        std::string sampleDataKey = outputLabel_ + "_SampleData";
        if (getDataProductManager()->hasProduct(sampleDataKey)) {
            auto sampleHandle = getDataProductManager()->checkoutWrite(sampleDataKey);
            sampleDataMap = dynamic_cast<DynamicSampleDataMap*>(sampleHandle->getObject());
        }
    }

    for (const TObject* obj : *inputList) {
        auto* wi = dynamic_cast<const WaveformIntegral*>(obj);
        if (!wi) continue;

        std::string key = "crate_" + std::to_string(wi->crateNum)
                        + "_amc_" + std::to_string(wi->amcNum)
                        + "_ch_" + std::to_string(wi->channelTag)
                        + "_det_" + wi->detectorSystem
                        + "_subdet_" + wi->subdetector;

        TH1D* hist = dynamic_cast<TH1D*>(outputList->FindObject(key.c_str()));

        if (useDynamicRange_ && sampleDataMap) {
            // Dynamic range mode
            auto& sampleData = sampleDataMap->data[key];
            
            if (!sampleData.histogramCreated) {
                // Still collecting samples
                if (sampleData.samples.size() < static_cast<size_t>(dynamicSampleSize_)) {
                    sampleData.samples.push_back(wi->integral);
                    continue; // Don't create histogram yet
                } else {
                    // We have enough samples, create the histogram
                    std::string histTitle = titlePrefix_ + " - Crate " + std::to_string(wi->crateNum)
                                                          + ", AMC " + std::to_string(wi->amcNum)
                                                          + ", Ch " + std::to_string(wi->channelTag)
                                                          + ", Det " + wi->detectorSystem
                                                          + ", Subdet " + wi->subdetector;
                    
                    CreateDynamicHistogram(outputList, key, sampleData.samples, histTitle);
                    sampleData.histogramCreated = true;
                    hist = dynamic_cast<TH1D*>(outputList->FindObject(key.c_str()));
                    
                    // Fill histogram with all collected samples
                    for (double sample : sampleData.samples) {
                        hist->Fill(sample);
                    }
                    // Clear samples to save memory
                    sampleData.samples.clear();
                    sampleData.samples.shrink_to_fit();
                }
            }
            
            // Fill current integral if histogram exists
            if (hist) {
                hist->Fill(wi->integral);
            }
        } else {
            // Original fixed/relative range modes
            if (!hist) {
                double histMin, histMax;

                if (useRelativeRange_) {
                    if (firstValueMap.find(key) == firstValueMap.end())
                        firstValueMap[key] = wi->integral;

                    double base = firstValueMap[key];
                    histMin = base + relativeMin_;
                    histMax = base + relativeMax_;
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
                outputList->Add(hist);
            }

            hist->Fill(wi->integral);
        }
    }
}

void WFD5WaveformIntegralHistogramStage::CreateDynamicHistogram(TList* outputList, 
                                                               const std::string& key,
                                                               const std::vector<double>& samples, 
                                                               const std::string& histTitle) {
    auto [mean, sigma] = CalculateStats(samples);
    
    double histMin = mean + dynamicMeanOffset_ - dynamicSigmaMultiplier_ * sigma;
    double histMax = mean + dynamicMeanOffset_ + dynamicSigmaMultiplier_ * sigma;
    
    // Ensure we have a valid range
    if (histMin == histMax) {
        histMax = histMin + 1.0;
    }
    
    TH1D* hist = new TH1D(key.c_str(), histTitle.c_str(), bins_, histMin, histMax);
    hist->SetDirectory(nullptr);
    outputList->Add(hist);
    
    spdlog::debug("[{}] Created dynamic histogram '{}': mean={:.2f}, sigma={:.2f}, range=[{:.2f}, {:.2f}]", 
                 Name(), key, mean, sigma, histMin, histMax);
}

std::pair<double, double> WFD5WaveformIntegralHistogramStage::CalculateStats(const std::vector<double>& samples) {
    if (samples.empty()) return {0.0, 1.0};
    
    // Calculate mean
    double sum = 0.0;
    for (double value : samples) {
        sum += value;
    }
    double mean = sum / samples.size();
    
    // Calculate standard deviation
    double sumSquaredDiff = 0.0;
    for (double value : samples) {
        double diff = value - mean;
        sumSquaredDiff += diff * diff;
    }
    double variance = sumSquaredDiff / samples.size();
    double sigma = std::sqrt(variance);
    
    // Avoid zero sigma
    if (sigma == 0.0) sigma = 1.0;
    
    return {mean, sigma};
}