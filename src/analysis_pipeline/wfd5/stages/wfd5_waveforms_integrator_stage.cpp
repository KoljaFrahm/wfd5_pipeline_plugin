#include "analysis_pipeline/wfd5/stages/wfd5_waveforms_integrator_stage.h"

#include <numeric>
#include <spdlog/spdlog.h>
#include <TList.h>

#include "data_products/wfd5/WFD5Waveform.hh"
#include "analysis_pipeline/wfd5/data_products/wfd5_trace_integral.h"

using namespace dataProducts;

ClassImp(WFD5WaveformsIntegratorStage)

WFD5WaveformsIntegratorStage::WFD5WaveformsIntegratorStage() = default;

void WFD5WaveformsIntegratorStage::OnInit() {
    inputLabel_ = parameters_.value("input_product", "WFD5WaveformCollection");
    outputLabel_ = parameters_.value("product_name", "WFD5TraceIntegralCollection");

    spdlog::debug("[{}] Initialized with input='{}', output='{}'",
                  Name(), inputLabel_, outputLabel_);
}

void WFD5WaveformsIntegratorStage::Process() {
    if (!getDataProductManager()->hasProduct(inputLabel_)) {
        spdlog::warn("[{}] Input product '{}' not found", Name(), inputLabel_);
        return;
    }

    auto list = std::make_unique<TList>();
    list->SetOwner(kTRUE);  // Ensures deletion of contained WFD5TraceIntegral objects

    try {
        auto lock = getDataProductManager()->checkoutRead(inputLabel_);
        const auto* waveformList = dynamic_cast<const TList*>(lock->getObject());

        if (!waveformList) {
            spdlog::error("[{}] Failed to cast input to TList", Name());
            return;
        }

        int count = 0;
        for (const TObject* obj : *waveformList) {
            auto* waveform = dynamic_cast<const WFD5Waveform*>(obj);
            if (!waveform) continue;

            double sum = std::accumulate(waveform->trace.begin(), waveform->trace.end(), 0.0);
            auto* integral = new WFD5TraceIntegral(
                waveform->crateNum,
                waveform->amcNum,
                waveform->channelTag,
                sum
            );

            list->Add(integral);
            ++count;
        }

        spdlog::debug("[{}] Integrated {} waveforms into TList", Name(), count);
    } catch (const std::exception& e) {
        spdlog::error("[{}] Exception while reading '{}': {}", Name(), inputLabel_, e.what());
        return;
    }

    auto pdp = std::make_unique<PipelineDataProduct>();
    pdp->setName(outputLabel_);
    pdp->setObject(std::move(list));
    pdp->addTag("WFD5");
    pdp->addTag("trace_integral");
    pdp->addTag("crate_amc_channel");
    pdp->addTag("sum");
    pdp->addTag("integral_list");
    pdp->addTag("built_by_wfd5_waveforms_integrator");
    getDataProductManager()->addOrUpdate(outputLabel_, std::move(pdp));
}
