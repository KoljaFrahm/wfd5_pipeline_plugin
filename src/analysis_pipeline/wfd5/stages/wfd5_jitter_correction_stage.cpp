#include "analysis_pipeline/wfd5/stages/wfd5_jitter_correction_stage.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <cmath>

#include <nlohmann/json.hpp> // you must have json.hpp included and configured

#include <spdlog/spdlog.h>
#include <TList.h>

using json = nlohmann::json;
using namespace dataProducts;

WFD5JitterCorrectionStage::WFD5JitterCorrectionStage()
    : BaseStage(), jitterCorrections_()
{}

bool WFD5JitterCorrectionStage::LoadPedestalFile(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs) {
        spdlog::error("[{}] Failed to open pedestal file: {}", Name(), filename);
        return false;
    }

    json j;
    try {
        ifs >> j;
    } catch (const std::exception& e) {
        spdlog::error("[{}] Failed to parse JSON: {}", Name(), e.what());
        return false;
    }

    if (!j.contains("pedestals") || !j["pedestals"].is_array()) {
        spdlog::error("[{}] JSON does not contain 'pedestals' array", Name());
        return false;
    }

    for (const auto& entry : j["pedestals"]) {
        if (!(entry.contains("crateNum") && entry.contains("amcSlotNum") && entry.contains("channelNum") && entry.contains("pedestal"))) {
            spdlog::warn("[{}] Incomplete pedestal entry skipped", Name());
            continue;
        }
        int crate = entry["crateNum"].get<int>();
        int amc = entry["amcSlotNum"].get<int>();
        int channel = entry["channelNum"].get<int>();
        int pedestal = static_cast<int>(std::round(entry["pedestal"].get<double>()));
        jitterCorrections_[std::make_tuple(crate, amc, channel)] = pedestal;
    }

    spdlog::info("[{}] Loaded {} pedestal entries from {}", Name(), jitterCorrections_.size(), filename);
    return true;
}

void WFD5JitterCorrectionStage::OnInit() {
    inputLabel_ = parameters_.value("input_product", "WFD5WaveformCollection");
    pedestalFilePath_ = parameters_.value("pedestal_file", "pedestals.json");

    if (!LoadPedestalFile(pedestalFilePath_)) {
        spdlog::error("[{}] Failed to load pedestal file '{}'", Name(), pedestalFilePath_);
    }
}

void WFD5JitterCorrectionStage::Process() {
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

    int corrected = 0;
    for (TObject* obj : *waveformList) {
        auto* wf = dynamic_cast<WFD5Waveform*>(obj);
        if (!wf) continue;

        ChannelKey key = std::make_tuple(wf->crateNum, wf->amcNum, wf->channelTag);

        auto it = jitterCorrections_.find(key);
        if (it == jitterCorrections_.end()) {
            spdlog::warn("[{}] No jitter correction found for channel (crate={}, amc={}, chan={})",
                         Name(), std::get<0>(key), std::get<1>(key), std::get<2>(key));
            continue;
        }

        int jitter_correction = it->second;
        wf->JitterCorrect(jitter_correction);
        ++corrected;
    }

    spdlog::debug("[{}] Applied jitter correction to {} waveforms", Name(), corrected);
}
