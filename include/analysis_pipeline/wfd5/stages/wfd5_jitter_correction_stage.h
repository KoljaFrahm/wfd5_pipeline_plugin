#ifndef WFD5_PIPELINE_PLUGIN_STAGES_WFD5_JITTER_CORRECTION_STAGE_H
#define WFD5_PIPELINE_PLUGIN_STAGES_WFD5_JITTER_CORRECTION_STAGE_H

#include "analysis_pipeline/core/stages/base_stage.h"
#include "data_products/wfd5/WFD5Waveform.hh"

#include <string>
#include <unordered_map>

class WFD5JitterCorrectionStage : public BaseStage {
public:
    WFD5JitterCorrectionStage();
    ~WFD5JitterCorrectionStage() override = default;

    void OnInit() override;
    void Process() override;

    std::string Name() const override { return "WFD5JitterCorrectionStage"; }

private:
    using ChannelKey = std::tuple<int,int,int>;  // (crateNum, amcSlotNum, channelTag)
    std::unordered_map<ChannelKey, int, 
        std::hash<std::string>> jitterCorrections_;

    std::string inputLabel_;
    std::string pedestalFilePath_;

    bool LoadPedestalFile(const std::string& filename);

    // Hash function for ChannelKey tuple to be defined or use a custom hasher

    ClassDefOverride(WFD5JitterCorrectionStage, 1);
};

#endif // WFD5_PIPELINE_PLUGIN_STAGES_WFD5_JITTER_CORRECTION_STAGE_H
