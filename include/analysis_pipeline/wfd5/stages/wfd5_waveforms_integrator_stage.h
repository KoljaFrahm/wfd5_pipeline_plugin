#ifndef WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORMS_INTEGRATOR_STAGE_H
#define WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORMS_INTEGRATOR_STAGE_H

#include "analysis_pipeline/core/stages/base_stage.h"

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

    ClassDefOverride(WFD5WaveformsIntegratorStage, 1);
};

#endif // WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORMS_INTEGRATOR_STAGE_H
