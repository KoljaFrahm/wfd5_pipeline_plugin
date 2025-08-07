#include "analysis_pipeline/wfd5/stages/wfd5_hodoscope_event_builder_stage.h"

#include <spdlog/spdlog.h>

#include "data_products/wfd5/WFD5Waveform.hh"
#include "analysis_pipeline/wfd5/data_products/wfd5_trace_integral.h"
#include "analysis_pipeline/wfd5/data_products/hodoscope_event.h"
#include <TList.h>

using namespace dataProducts;

ClassImp(WFD5HodoscopeEventBuilderStage)

WFD5HodoscopeEventBuilderStage::WFD5HodoscopeEventBuilderStage() = default;

void WFD5HodoscopeEventBuilderStage::OnInit() {
    integralInputLabel_ = parameters_.value("integral_input", "WFD5TraceIntegralCollection");
    waveformInputLabel_ = parameters_.value("waveform_input", "WFD5WaveformCollection");
    outputLabel_ = parameters_.value("product_name", "HodoscopeEvent");
    targetDetectorSystem_ = parameters_.value("detector_system", "HODO");

    spdlog::debug("[{}] Initialized with integrals='{}', waveforms='{}', output='{}', detector='{}'",
                  Name(), integralInputLabel_, waveformInputLabel_, outputLabel_, targetDetectorSystem_);
}

void WFD5HodoscopeEventBuilderStage::Process() {
    // Verify that input data products exist before proceeding
    if (!getDataProductManager()->hasProduct(integralInputLabel_) ||
        !getDataProductManager()->hasProduct(waveformInputLabel_)) {
        spdlog::warn("[{}] Missing input product(s): '{}' or '{}'",
                     Name(), integralInputLabel_, waveformInputLabel_);
        return;
    }

    try {
        // Acquire read locks to safely access input products
        auto lockIntegral = getDataProductManager()->checkoutRead(integralInputLabel_);
        auto lockWaveform = getDataProductManager()->checkoutRead(waveformInputLabel_);

        // Extract the raw TList objects from the data products
        const auto* integrals = dynamic_cast<const TList*>(lockIntegral->getObject());
        const auto* waveforms = dynamic_cast<const TList*>(lockWaveform->getObject());

        if (!integrals || !waveforms) {
            spdlog::error("[{}] Failed to cast inputs to TList", Name());
            return;
        }

        // Create a map for fast waveform lookup keyed by (crate, amc, channel)
        std::map<std::tuple<int, int, uint64_t>, const WFD5Waveform*> waveformMap;
        for (const TObject* obj : *waveforms) {
            const auto* wf = dynamic_cast<const WFD5Waveform*>(obj);
            if (!wf) continue;
            waveformMap[{wf->crateNum, wf->amcNum, wf->channelTag}] = wf;
        }

        // Initialize a single event object to hold max integral and position info for X and Y
        auto evt = std::make_unique<HodoscopeEvent>();
        evt->max_integral_x = -1e12;  // Sentinel low value for max comparison
        evt->max_integral_y = -1e12;

        // Iterate over integral objects to find max integral channels for X and Y
        for (const TObject* obj : *integrals) {
            const auto* integ = dynamic_cast<const WFD5TraceIntegral*>(obj);
            if (!integ) continue;

            // Find the matching waveform using crate, amc, and channel keys
            auto key = std::make_tuple(integ->crateNum, integ->amcNum, integ->channelNum);
            auto it = waveformMap.find(key);
            if (it == waveformMap.end()) continue;

            const auto* wf = it->second;
            // Restrict to the configured target detector system
            if (wf->detectorSystem != targetDetectorSystem_) continue;

            const std::string& subdet = wf->subdetector;

            // Determine if the channel is X or Y based on subdetector string (case insensitive)
            bool isX = (subdet.find('X') != std::string::npos) || (subdet.find('x') != std::string::npos);
            bool isY = (subdet.find('Y') != std::string::npos) || (subdet.find('y') != std::string::npos);

            // Ensure channel is exclusively X or Y, skip ambiguous or unknown
            if (isX == isY) {
                spdlog::warn("[{}] Channel at crate={}, amc={}, channel={} has ambiguous or missing subdetector '{}'; skipping",
                             Name(), wf->crateNum, wf->amcNum, wf->channelTag, subdet);
                continue;
            }

            // Update max integral and associated metadata for X channels
            if (isX) {
                if (integ->integralValue > evt->max_integral_x) {
                    evt->max_integral_x = integ->integralValue;
                    evt->max_channel_x = integ->channelNum;
                    evt->max_amc_x = integ->amcNum;
                    evt->max_crate_x = integ->crateNum;
                    evt->max_x = wf->x;
                }
            }
            // Update max integral and associated metadata for Y channels
            else {
                if (integ->integralValue > evt->max_integral_y) {
                    evt->max_integral_y = integ->integralValue;
                    evt->max_channel_y = integ->channelNum;
                    evt->max_amc_y = integ->amcNum;
                    evt->max_crate_y = integ->crateNum;
                    evt->max_y = wf->y;
                }
            }
        }

        // Directly set the single HodoscopeEvent as the data product object
        auto pdp = std::make_unique<PipelineDataProduct>();
        pdp->setName(outputLabel_);
        pdp->setObject(std::move(evt));
        pdp->addTag("hodoscope_event");
        pdp->addTag("WFD5");
        pdp->addTag("integral_waveform_combined");
        pdp->addTag("crate_amc_channel");
        pdp->addTag("filtered_" + targetDetectorSystem_);
        getDataProductManager()->addOrUpdate(outputLabel_, std::move(pdp));
    } catch (const std::exception& e) {
        spdlog::error("[{}] Exception during processing: {}", Name(), e.what());
    }
}
