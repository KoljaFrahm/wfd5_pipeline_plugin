#ifndef WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORM_INTEGRAL_HISTOGRAM_STAGE_H
#define WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORM_INTEGRAL_HISTOGRAM_STAGE_H

#include "analysis_pipeline/core/stages/base_stage.h"
#include <string>
#include <vector>
#include <unordered_map>

/**
 * @class DynamicSampleData
 * @brief TObject-derived class to store sample data for dynamic histogram binning
 */
class DynamicSampleData : public TObject {
public:
    std::vector<double> samples;
    bool histogramCreated = false;
    
    DynamicSampleData() = default;
    ~DynamicSampleData() override = default;
    
    ClassDefOverride(DynamicSampleData, 1);
};

/**
 * @class DynamicSampleDataMap  
 * @brief TObject-derived map to store DynamicSampleData objects
 */
class DynamicSampleDataMap : public TObject {
public:
    std::unordered_map<std::string, DynamicSampleData> data;
    
    DynamicSampleDataMap() = default;
    ~DynamicSampleDataMap() override = default;
    
    ClassDefOverride(DynamicSampleDataMap, 1);
};

/**
 * @class WFD5WaveformIntegralHistogramStage
 * @brief Pipeline stage reading a TList of WaveformIntegral objects
 *        and accumulating one TH1D histogram per {crate, amc, channel} triple.
 *
 * Histogram range can be fixed, relative to the first sample value, or dynamic
 * based on statistical analysis of initial samples.
 */
class WFD5WaveformIntegralHistogramStage : public BaseStage {
public:
    WFD5WaveformIntegralHistogramStage() = default;
    ~WFD5WaveformIntegralHistogramStage() override = default;

    void OnInit() override;
    void Process() override;

    std::string Name() const override { return "WFD5WaveformIntegralHistogramStage"; }

private:
    std::string inputLabel_;
    std::string outputLabel_;
    std::string titlePrefix_;
    int bins_ = 100;

    // Existing range modes
    bool useRelativeRange_ = false;
    double relativeMin_ = 0.0;
    double relativeMax_ = 0.0;
    double min_ = 0.0;
    double max_ = 10000.0;

    // Dynamic binning parameters
    bool useDynamicRange_ = false;
    int dynamicSampleSize_ = 100;
    double dynamicMeanOffset_ = 0.0;
    double dynamicSigmaMultiplier_ = 3.0;

    void FillHistograms(TList* outputList, const TList* inputList);
    void CreateDynamicHistogram(TList* outputList, const std::string& key, 
                               const std::vector<double>& samples, 
                               const std::string& histTitle);
    std::pair<double, double> CalculateStats(const std::vector<double>& samples);

    ClassDefOverride(WFD5WaveformIntegralHistogramStage, 1);
};