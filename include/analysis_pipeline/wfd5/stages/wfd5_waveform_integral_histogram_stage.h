#ifndef WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORM_INTEGRAL_HISTOGRAM_STAGE_H
#define WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORM_INTEGRAL_HISTOGRAM_STAGE_H

#include "analysis_pipeline/core/stages/base_stage.h"
#include <string>

/**
 * @class WFD5WaveformIntegralHistogramStage
 * @brief Pipeline stage reading a TList of WaveformIntegral objects
 *        and accumulating one TH1D histogram per {crate, amc, channel} triple.
 *
 * Histogram range can be fixed or relative to the first sample value.
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

    bool useRelativeRange_ = false;
    double relativeMin_ = 0.0;
    double relativeMax_ = 0.0;
    double min_ = 0.0;
    double max_ = 10000.0;

    void FillHistograms(TList* outputList, const TList* inputList);

    ClassDefOverride(WFD5WaveformIntegralHistogramStage, 1);
};

#endif // WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORM_INTEGRAL_HISTOGRAM_STAGE_H
