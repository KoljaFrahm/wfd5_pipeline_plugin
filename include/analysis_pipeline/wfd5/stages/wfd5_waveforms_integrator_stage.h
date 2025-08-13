#ifndef WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORMS_INTEGRATOR_STAGE_H
#define WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORMS_INTEGRATOR_STAGE_H

#include "analysis_pipeline/core/stages/base_stage.h"
#include "data_products/wfd5/WFD5Waveform.hh"
#include <string>

class WFD5WaveformsIntegratorStage : public BaseStage {
public:
    enum class IntegrationMode {
        All,
        AboutMax,
        AboutFixed
    };

    WFD5WaveformsIntegratorStage();
    ~WFD5WaveformsIntegratorStage() override = default;

    void Process() override;
    std::string Name() const override { return "WFD5WaveformsIntegratorStage"; }

protected:
    void OnInit() override;

private:
    std::string inputLabel_;
    std::string outputLabel_;
    IntegrationMode mode_ = IntegrationMode::All;
    int presamples_ = 0;
    int integralLength_ = 0;

    double integrateAll(const dataProducts::WFD5Waveform* wf) const;
    double integrateAboutMax(const dataProducts::WFD5Waveform* wf) const;
    double integrateAboutFixed(const dataProducts::WFD5Waveform* wf) const; // currently uses AboutMax

    ClassDefOverride(WFD5WaveformsIntegratorStage, 2);
};

#endif // WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORMS_INTEGRATOR_STAGE_H
