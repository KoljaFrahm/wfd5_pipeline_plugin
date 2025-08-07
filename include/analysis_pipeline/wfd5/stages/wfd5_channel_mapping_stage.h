#ifndef WFD5_CHANNEL_MAPPING_STAGE_H
#define WFD5_CHANNEL_MAPPING_STAGE_H

#include "analysis_pipeline/core/stages/base_stage.h"
#include "data_products/wfd5/WFD5Waveform.hh"

#include <string>
#include <map>
#include <tuple>

class WFD5ChannelMappingStage : public BaseStage {
public:
    WFD5ChannelMappingStage();
    ~WFD5ChannelMappingStage() override = default;

    void OnInit() override;
    void Process() override;
    std::string Name() const override { return "WFD5ChannelMappingStage"; }

    struct ChannelInfo {
        std::string detectorSystem = "";
        std::string subdetector = "";
        double x = 0.0;
        double y = 0.0;
    };


private:
    std::string inputLabel_;
    std::string mapFilePath_;

    using ChannelKey = std::tuple<int, int, int>;
    std::map<ChannelKey, ChannelInfo> channelMap_;

    void LoadChannelMap(const std::string& path);
    ChannelKey MakeKey(int crate, int amc, int ch) const;

    ClassDefOverride(WFD5ChannelMappingStage, 1);
};

#endif // WFD5_CHANNEL_MAPPING_STAGE_H
