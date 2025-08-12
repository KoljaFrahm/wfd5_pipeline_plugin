#ifndef WFD5_PIPELINE_PLUGIN_STAGES_WFD5_JITTER_CORRECTION_STAGE_H
#define WFD5_PIPELINE_PLUGIN_STAGES_WFD5_JITTER_CORRECTION_STAGE_H

#include "analysis_pipeline/core/stages/base_stage.h"
#include "data_products/wfd5/WFD5Waveform.hh"

#include <string>
#include <map>
#include <tuple>

class WFD5JitterCorrectionStage : public BaseStage {
public:
    WFD5JitterCorrectionStage();
    ~WFD5JitterCorrectionStage() override = default;

    void OnInit() override;
    void Process() override;

    std::string Name() const override { return "WFD5JitterCorrectionStage"; }

private:
    using ChannelKey = std::tuple<int,int,int>;  // (crateNum, amcSlotNum, channelTag)
    std::map<ChannelKey, int> jitterCorrections_;  // Use std::map instead of unordered_map

    std::string inputLabel_;
    std::string pedestalFilePath_;

    bool LoadPedestalFile(const std::string& filename);

    ClassDefOverride(WFD5JitterCorrectionStage, 1);
};

#endif // WFD5_PIPELINE_PLUGIN_STAGES_WFD5_JITTER_CORRECTION_STAGE_H
