#ifndef WFD5_PIPELINE_PLUGIN_STAGES_WFD5_HODOSCOPE_EVENT_BUILDER_STAGE_H
#define WFD5_PIPELINE_PLUGIN_STAGES_WFD5_HODOSCOPE_EVENT_BUILDER_STAGE_H

#include "analysis_pipeline/core/stages/base_stage.h"

class WFD5HodoscopeEventBuilderStage : public BaseStage {
public:
    WFD5HodoscopeEventBuilderStage();
    ~WFD5HodoscopeEventBuilderStage() override = default;

    void Process() override;
    std::string Name() const override { return "WFD5HodoscopeEventBuilderStage"; }

protected:
    void OnInit() override;

private:
    std::string integralInputLabel_;
    std::string waveformInputLabel_;
    std::string outputLabel_;
    std::string targetDetectorSystem_;  // e.g., "HODO"

    ClassDefOverride(WFD5HodoscopeEventBuilderStage, 1);
};

#endif  // WFD5_PIPELINE_PLUGIN_STAGES_WFD5_HODOSCOPE_EVENT_BUILDER_STAGE_H
