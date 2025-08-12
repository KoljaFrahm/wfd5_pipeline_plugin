// wfd5_pedestal_correction_stage.cpp
#include "analysis_pipeline/wfd5/stages/wfd5_pedestal_correction_stage.h"

#include <spdlog/spdlog.h>
#include <numeric>
#include <algorithm>
#include <TList.h>

using namespace dataProducts;

ClassImp(WFD5PedestalCorrectionStage)

WFD5PedestalCorrectionStage::WFD5PedestalCorrectionStage() = default;

void WFD5PedestalCorrectionStage::OnInit() {
    inputLabel_ = parameters_.value("input_product", "WFD5WaveformCollection");
    nsamples_ = parameters_.value("pedestal_nsamples", 10);
    std::string methodStr = ToLower(parameters_.value("pedestal_method", "first"));

    if (methodStr == "first") {
        method_ = PedestalMethod::First;
    } else if (methodStr == "min") {
        method_ = PedestalMethod::Min;
    } else if (methodStr == "average") {
        method_ = PedestalMethod::Average;
    } else {
        spdlog::warn("[{}] Unknown pedestal_method '{}', defaulting to 'first'", Name(), methodStr);
        method_ = PedestalMethod::First;
    }

    spdlog::debug("[{}] Initialized with input='{}', nsamples={}, method='{}'",
                  Name(), inputLabel_, nsamples_, methodStr);
}

void WFD5PedestalCorrectionStage::Process() {
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

        CorrectPedestal(*wf);
        ++corrected;
    }

    spdlog::debug("[{}] Corrected pedestal for {} waveforms", Name(), corrected);
}

void WFD5PedestalCorrectionStage::CorrectPedestal(WFD5Waveform& wf) {
    const std::vector<short>& trace = wf.trace;
    if (trace.size() < static_cast<size_t>(nsamples_)) {
        spdlog::warn("[{}] Waveform too short to correct pedestal (size={})", Name(), trace.size());
        return;
    }

    // Compute pedestal as average of first nsamples_ samples
    double pedestal = std::accumulate(trace.begin(), trace.begin() + nsamples_, 0.0) / nsamples_;

    // Optional: compute pedestal stddev for info
    double accum = 0.0;
    for (size_t i = 0; i < static_cast<size_t>(nsamples_); ++i) {
        double diff = static_cast<double>(trace[i]) - pedestal;
        accum += diff * diff;
    }
    double stdev = std::sqrt(accum / (nsamples_ - 1));

    wf.pedestalLevel = pedestal;
    wf.pedestalStdev = stdev;

    // Subtract pedestal from all samples
    for (short& sample : wf.trace) {
        sample = static_cast<short>(std::round(static_cast<double>(sample) - pedestal));
    }

    // Compute sum of corrected trace samples
    int64_t sum_samples = 0;
    for (const auto& s : wf.trace) {
        sum_samples += s;
    }

    // Print sum for debugging (replace with spdlog if preferred)
    spdlog::info("[{}] Pedestal level: {:.3f}, pedestal stddev: {:.3f}", Name(), pedestal, stdev);

    std::string sample_str;
    for (size_t i = 0; i < wf.trace.size(); ++i) {
        sample_str += std::to_string(wf.trace[i]);
        if (i != wf.trace.size() - 1)
            sample_str += ", ";
    }
    spdlog::info("[{}] Corrected samples: [{}]", Name(), sample_str);

    spdlog::info("[{}] Corrected trace sum: {}", Name(), sum_samples);

}


std::string WFD5PedestalCorrectionStage::ToLower(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) out.push_back(std::tolower(c));
    return out;
}
