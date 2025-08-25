#ifndef WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORM_INTEGRAL_HISTOGRAM_STAGE_H
#define WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORM_INTEGRAL_HISTOGRAM_STAGE_H

#include "analysis_pipeline/core/stages/base_stage.h"
#include <string>
#include <unordered_map>
#include <vector>

class TList;

/**
 * @class WFD5WaveformIntegralHistogramStage
 * @brief Pipeline stage reading a TList of WaveformIntegral objects
 *        and accumulating one TH1D histogram per {crate, amc, channel} triple.
 *
 * Histogram range can be:
 * - Fixed: user-specified min/max values
 * - Relative: relative to first sample value 
 * - Dynamic: determined from statistics of initial sample values
 */
class WFD5WaveformIntegralHistogramStage : public BaseStage {
public:
    WFD5WaveformIntegralHistogramStage() = default;
    ~WFD5WaveformIntegralHistogramStage() override = default;

    void OnInit() override;
    void Process() override;

    std::string Name() const override { return "WFD5WaveformIntegralHistogramStage"; }

private:
    // Configuration parameters
    std::string inputLabel_;
    std::string outputLabel_;
    std::string titlePrefix_;
    int bins_ = 100;

    // Range mode selection
    enum class RangeMode { FIXED, RELATIVE, DYNAMIC };
    RangeMode rangeMode_ = RangeMode::FIXED;

    // Fixed range parameters
    double min_ = 0.0;
    double max_ = 10000.0;

    // Relative range parameters
    double relativeMin_ = 0.0;
    double relativeMax_ = 0.0;

    // Dynamic range parameters
    int dynamicSampleSize_ = 100;
    double dynamicMeanOffset_ = 0.0;
    double dynamicSigmaMultiplier_ = 3.0;

    // Dynamic range state tracking
    std::unordered_map<std::string, std::vector<double>> sampleValues_;
    std::unordered_map<std::string, bool> rangeComputed_;
    std::unordered_map<std::string, double> computedMin_;
    std::unordered_map<std::string, double> computedMax_;
    std::unordered_map<std::string, double> firstValueMap_;

    void FillHistograms(TList* outputList, const TList* inputList);
    void ComputeDynamicRange(const std::string& key, const std::vector<double>& samples);
    std::pair<double, double> GetHistogramRange(const std::string& key, double currentValue);

    ClassDefOverride(WFD5WaveformIntegralHistogramStage, 1);
};

#endif // WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORM_INTEGRAL_HISTOGRAM_STAGE_H