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
    if (trace.size() < static_cast<size_t>(2 * nsamples_)) {
        spdlog::warn("[{}] Waveform too short to correct pedestal (size={})", Name(), trace.size());
        return;
    }

    std::vector<double> pedestals, stdevs;

    std::vector<size_t> offsets = {0, trace.size() - static_cast<size_t>(nsamples_)};

    for (size_t offset : offsets) {
        double mean = std::accumulate(trace.begin() + offset,
                                      trace.begin() + offset + nsamples_, 0.0) / nsamples_;
        double accum = 0.0;
        for (size_t i = offset; i < offset + nsamples_; ++i) {
            double diff = static_cast<double>(trace[i]) - mean;
            accum += diff * diff;
        }
        double stdev = std::sqrt(accum / (nsamples_ - 1));
        pedestals.push_back(mean);
        stdevs.push_back(stdev);
    }

    switch (method_) {
        case PedestalMethod::First:
            wf.pedestalLevel = pedestals[0];
            wf.pedestalStdev = stdevs[0];
            break;
        case PedestalMethod::Min: {
            auto it = std::min_element(pedestals.begin(), pedestals.end());
            size_t idx = std::distance(pedestals.begin(), it);
            wf.pedestalLevel = *it;
            wf.pedestalStdev = stdevs[idx];
            break;
        }
        case PedestalMethod::Average:
            wf.pedestalLevel = 0.5 * (pedestals[0] + pedestals[1]);
            wf.pedestalStdev = 0.5 * (stdevs[0] + stdevs[1]);
            break;
    }

    for (short& sample : wf.trace) {
        sample = static_cast<short>(std::round(static_cast<double>(sample) - wf.pedestalLevel));
    }
}

std::string WFD5PedestalCorrectionStage::ToLower(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) out.push_back(std::tolower(c));
    return out;
}
