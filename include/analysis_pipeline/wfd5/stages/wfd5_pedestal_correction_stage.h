#ifndef WFD5_PIPELINE_PLUGIN_STAGES_WFD5_PEDESTAL_CORRECTION_STAGE_H
#define WFD5_PIPELINE_PLUGIN_STAGES_WFD5_PEDESTAL_CORRECTION_STAGE_H

#include "analysis_pipeline/core/stages/base_stage.h"
#include "data_products/wfd5/WFD5Waveform.hh"
#include <string>

class WFD5PedestalCorrectionStage : public BaseStage {
public:
    WFD5PedestalCorrectionStage();
    ~WFD5PedestalCorrectionStage() override = default;

    void OnInit() override;
    void Process() override;

    std::string Name() const override { return "WFD5PedestalCorrectionStage"; }

private:
    enum class PedestalMethod {
        First,
        Min,
        AverageClosest
    };

    std::string inputLabel_;
    int nsamples_ = 10;
    PedestalMethod method_ = PedestalMethod::First;

    void CorrectPedestal(dataProducts::WFD5Waveform& wf);
    static std::string ToLower(const std::string& s);

    ClassDefOverride(WFD5PedestalCorrectionStage, 1);
};

#endif // WFD5_PIPELINE_PLUGIN_STAGES_WFD5_PEDESTAL_CORRECTION_STAGE_H
