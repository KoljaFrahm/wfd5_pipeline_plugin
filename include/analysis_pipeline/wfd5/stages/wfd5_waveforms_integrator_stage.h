#ifndef WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORMS_INTEGRATOR_STAGE_H
#define WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORMS_INTEGRATOR_STAGE_H

#include "analysis_pipeline/core/stages/base_stage.h"
#include "data_products/wfd5/WFD5Waveform.hh"
#include "data_products/wfd5/WaveformIntegral.hh"
#include <string>

class WFD5WaveformsIntegratorStage : public BaseStage {
public:
    WFD5WaveformsIntegratorStage();
    ~WFD5WaveformsIntegratorStage() override = default;

    void Process() override;
    std::string Name() const override { return "WFD5WaveformsIntegratorStage"; }

protected:
    void OnInit() override;

private:
    std::string inputLabel_;
    std::string outputLabel_;

    // Integration parameters
    double nsigma_ = 10.0;
    int searchMethod_ = 0;
    std::pair<int,int> presampleConfig_ = {10, 250};
    int seedIndex_ = -1;
    int seededSearchWindow_ = -1;

    ClassDefOverride(WFD5WaveformsIntegratorStage, 3);
};

#endif // WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORMS_INTEGRATOR_STAGE_H
