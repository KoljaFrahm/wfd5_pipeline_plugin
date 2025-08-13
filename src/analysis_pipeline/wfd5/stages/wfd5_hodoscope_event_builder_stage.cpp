#include "analysis_pipeline/wfd5/stages/wfd5_hodoscope_event_builder_stage.h"

#include <spdlog/spdlog.h>
#include <map>
#include <tuple>

#include "data_products/wfd5/WFD5Waveform.hh"
#include "analysis_pipeline/wfd5/data_products/waveform_integral.h"
#include "analysis_pipeline/wfd5/data_products/hodoscope_event.h"
#include <TList.h>

using namespace dataProducts;

ClassImp(WFD5HodoscopeEventBuilderStage)

WFD5HodoscopeEventBuilderStage::WFD5HodoscopeEventBuilderStage() = default;

void WFD5HodoscopeEventBuilderStage::OnInit() {
    integralInputLabel_ = parameters_.value("integral_input", "WaveformIntegralCollection");
    waveformInputLabel_ = parameters_.value("waveform_input", "WFD5WaveformCollection");
    outputLabel_ = parameters_.value("product_name", "HodoscopeEvent");
    targetDetectorSystem_ = parameters_.value("detector_system", "HODO");

    spdlog::debug("[{}] Initialized with integrals='{}', waveforms='{}', output='{}', detector='{}'",
                  Name(), integralInputLabel_, waveformInputLabel_, outputLabel_, targetDetectorSystem_);
}

void WFD5HodoscopeEventBuilderStage::Process() {
    if (!getDataProductManager()->hasProduct(integralInputLabel_) ||
        !getDataProductManager()->hasProduct(waveformInputLabel_)) {
        spdlog::warn("[{}] Missing input product(s): '{}' or '{}'",
                     Name(), integralInputLabel_, waveformInputLabel_);
        return;
    }

    try {
        auto lockIntegral = getDataProductManager()->checkoutRead(integralInputLabel_);
        auto lockWaveform = getDataProductManager()->checkoutRead(waveformInputLabel_);

        const auto* integrals = dynamic_cast<const TList*>(lockIntegral->getObject());
        const auto* waveforms = dynamic_cast<const TList*>(lockWaveform->getObject());

        if (!integrals || !waveforms) {
            spdlog::error("[{}] Failed to cast inputs to TList", Name());
            return;
        }

        // Map key: crate, amc, channelTag (uint64_t)
        std::map<std::tuple<int,int,uint64_t>, const WFD5Waveform*> waveformMap;
        for (const TObject* obj : *waveforms) {
            const auto* wf = dynamic_cast<const WFD5Waveform*>(obj);
            if (!wf) continue;
            waveformMap[{wf->crateNum, wf->amcNum, wf->channelTag}] = wf;
        }

        auto evt = std::make_unique<HodoscopeEvent>();
        evt->max_integral_x = -1e12;
        evt->max_integral_y = -1e12;

        for (const TObject* obj : *integrals) {
            const auto* integ = dynamic_cast<const WaveformIntegral*>(obj);
            if (!integ) continue;

            auto key = std::make_tuple(integ->crateNum, integ->amcNum, integ->channelTag);
            auto it = waveformMap.find(key);
            if (it == waveformMap.end()) continue;

            const auto* wf = it->second;

            if (wf->detectorSystem != targetDetectorSystem_) continue;

            const std::string& subdet = wf->subdetector;

            bool isX = (subdet.find('X') != std::string::npos) || (subdet.find('x') != std::string::npos);
            bool isY = (subdet.find('Y') != std::string::npos) || (subdet.find('y') != std::string::npos);

            if (isX == isY) {
                spdlog::warn("[{}] Ambiguous subdetector '{}' at crate={}, amc={}, channel={}; skipping",
                             Name(), subdet, wf->crateNum, wf->amcNum, wf->channelTag);
                continue;
            }

            if (isX) {
                if (integ->integral > evt->max_integral_x) {
                    evt->max_integral_x = integ->integral;
                    evt->max_channel_x = integ->channelTag;
                    evt->max_amc_x = integ->amcNum;
                    evt->max_crate_x = integ->crateNum;
                    evt->max_x = wf->x;
                }
            } else {
                if (integ->integral > evt->max_integral_y) {
                    evt->max_integral_y = integ->integral;
                    evt->max_channel_y = integ->channelTag;
                    evt->max_amc_y = integ->amcNum;
                    evt->max_crate_y = integ->crateNum;
                    evt->max_y = wf->y;
                }
            }
        }

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
