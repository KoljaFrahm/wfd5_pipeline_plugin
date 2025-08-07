#include "analysis_pipeline/wfd5/stages/wfd5_channel_mapping_stage.h"

#include <fstream>
#include <TList.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include "data_products/wfd5/WFD5Waveform.hh"

using json = nlohmann::json;
using namespace dataProducts;

ClassImp(WFD5ChannelMappingStage)

WFD5ChannelMappingStage::WFD5ChannelMappingStage() = default;

void WFD5ChannelMappingStage::OnInit() {
    inputLabel_ = parameters_.value("input_product", "WFD5WaveformCollection");
    mapFilePath_ = parameters_.value("channel_map_file", "");

    if (mapFilePath_.empty()) {
        spdlog::error("[{}] No channel_map_file specified", Name());
        throw std::runtime_error("Missing required channel_map_file");
    }

    LoadChannelMap(mapFilePath_);
    spdlog::debug("[{}] Loaded {} channel mappings from '{}'", Name(), channelMap_.size(), mapFilePath_);
}

void WFD5ChannelMappingStage::LoadChannelMap(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Failed to open channel map file: " + path);
    }

    json j;
    in >> j;

    for (const auto& entry : j.at("channelMap")) {
        int crate = entry.value("crateNum", -1);
        int amc   = entry.value("amcSlotNum", -1);
        int ch    = entry.value("channelNum", -1);

        ChannelInfo info;
        info.detectorSystem = entry.value("detectorSystem", "");
        info.subdetector    = entry.value("subdetector", "");
        info.x              = entry.value("x", 0.0);
        info.y              = entry.value("y", 0.0);

        channelMap_[MakeKey(crate, amc, ch)] = info;
    }
}

WFD5ChannelMappingStage::ChannelKey
WFD5ChannelMappingStage::MakeKey(int crate, int amc, int ch) const {
    return std::make_tuple(crate, amc, ch);
}

void WFD5ChannelMappingStage::Process() {
    if (!getDataProductManager()->hasProduct(inputLabel_)) {
        spdlog::warn("[{}] Input product '{}' not found", Name(), inputLabel_);
        return;
    }

    auto handle = getDataProductManager()->checkoutWrite(inputLabel_);
    auto* waveformList = dynamic_cast<TList*>(handle->getObject());

    if (!waveformList) {
        spdlog::error("[{}] Input '{}' is not a TList", Name(), inputLabel_);
        return;
    }

    int updated = 0;
    for (TObject* obj : *waveformList) {
        auto* wf = dynamic_cast<WFD5Waveform*>(obj);
        if (!wf) continue;

        auto key = MakeKey(wf->crateNum, wf->amcNum, wf->channelTag);

        const auto it = channelMap_.find(key);
        if (it != channelMap_.end()) {
            const ChannelInfo& info = it->second;
            wf->detectorSystem = info.detectorSystem;
            wf->subdetector    = info.subdetector;
            wf->x = info.x;
            wf->y = info.y;
            ++updated;
        } else {
            wf->detectorSystem = "";
            wf->subdetector    = "";
            wf->x = 0.0;
            wf->y = 0.0;

            spdlog::debug("[{}] No mapping found for crate={}, amc={}, ch={}",
                          Name(), wf->crateNum, wf->amcNum, wf->channelTag);
        }
    }

    spdlog::debug("[{}] Updated {} waveform(s) with channel mapping metadata", Name(), updated);
}
