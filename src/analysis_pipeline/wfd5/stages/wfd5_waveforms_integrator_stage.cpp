#include "analysis_pipeline/wfd5/stages/wfd5_waveforms_integrator_stage.h"

#include <numeric>
#include <algorithm>
#include <spdlog/spdlog.h>
#include <TList.h>

#include "analysis_pipeline/wfd5/data_products/wfd5_trace_integral.h"

using namespace dataProducts;

ClassImp(WFD5WaveformsIntegratorStage)

WFD5WaveformsIntegratorStage::WFD5WaveformsIntegratorStage() = default;

void WFD5WaveformsIntegratorStage::OnInit() {
    inputLabel_ = parameters_.value("input_product", "WFD5WaveformCollection");
    outputLabel_ = parameters_.value("product_name", "WFD5TraceIntegralCollection");

    std::string modeStr = parameters_.value("integration_mode", "all");
    if (modeStr == "all") {
        mode_ = IntegrationMode::All;
    } else if (modeStr == "about_max") {
        mode_ = IntegrationMode::AboutMax;
    } else if (modeStr == "about_fixed") {
        mode_ = IntegrationMode::AboutFixed;
        spdlog::warn("[{}] AboutFixed mode not implemented, falling back to AboutMax", Name());
    } else {
        spdlog::error("[{}] Unknown integration_mode '{}', defaulting to All", Name(), modeStr);
        mode_ = IntegrationMode::All;
    }

    presamples_ = parameters_.value("presamples", 0);
    integralLength_ = parameters_.value("integral_length", 0);

    spdlog::debug("[{}] Initialized with input='{}', output='{}', mode={}, presamples={}, length={}",
                  Name(), inputLabel_, outputLabel_, modeStr, presamples_, integralLength_);
}

void WFD5WaveformsIntegratorStage::Process() {
    if (!getDataProductManager()->hasProduct(inputLabel_)) {
        spdlog::warn("[{}] Input product '{}' not found", Name(), inputLabel_);
        return;
    }

    auto list = std::make_unique<TList>();
    list->SetOwner(kTRUE);

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

            double integral = 0.0;
            switch (mode_) {
                case IntegrationMode::All:
                    integral = integrateAll(waveform);
                    break;
                case IntegrationMode::AboutMax:
                    integral = integrateAboutMax(waveform);
                    break;
                case IntegrationMode::AboutFixed:
                    integral = integrateAboutFixed(waveform);
                    break;
            }

            auto* ti = new WFD5TraceIntegral(
                waveform->crateNum,
                waveform->amcNum,
                waveform->channelTag,
                integral
            );
            list->Add(ti);
            ++count;
        }

        spdlog::debug("[{}] Integrated {} waveforms", Name(), count);
    } catch (const std::exception& e) {
        spdlog::error("[{}] Exception while processing '{}': {}", Name(), inputLabel_, e.what());
        return;
    }

    auto pdp = std::make_unique<PipelineDataProduct>();
    pdp->setName(outputLabel_);
    pdp->setObject(std::move(list));
    pdp->addTag("WFD5");
    pdp->addTag("trace_integral");
    pdp->addTag("crate_amc_channel");
    pdp->addTag("integral_list");
    pdp->addTag("built_by_wfd5_waveforms_integrator");
    getDataProductManager()->addOrUpdate(outputLabel_, std::move(pdp));
}

double WFD5WaveformsIntegratorStage::integrateAll(const WFD5Waveform* wf) const {
    double sum = std::accumulate(wf->trace.begin(), wf->trace.end(), 0.0);
    double pedestalSum = wf->pedestalLevel * wf->trace.size();
    return sum - pedestalSum;
}

double WFD5WaveformsIntegratorStage::integrateAboutMax(const WFD5Waveform* wf) const {
    if (integralLength_ <= 0) return integrateAll(wf);
    int peak = wf->GetPeakIndex();
    int start = std::max(0, peak - presamples_);
    int end = std::min<int>(wf->trace.size(), start + integralLength_);

    double sum = std::accumulate(wf->trace.begin() + start, wf->trace.begin() + end, 0.0);
    double pedestalSum = wf->pedestalLevel * (end - start);
    return sum - pedestalSum;
}

double WFD5WaveformsIntegratorStage::integrateAboutFixed(const WFD5Waveform* wf) const {
    // Not implemented: use AboutMax
    return integrateAboutMax(wf);
}
