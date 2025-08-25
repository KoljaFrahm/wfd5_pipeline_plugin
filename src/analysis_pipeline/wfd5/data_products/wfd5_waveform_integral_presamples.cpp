#include "analysis_pipeline/wfd5/data_products/wfd5_waveform_integral_presamples.h"

#include <numeric>
#include <cmath>
#include <sstream>
#include <iostream>

ClassImp(WFD5WaveformIntegralPresamples)

WFD5WaveformIntegralPresamples::WFD5WaveformIntegralPresamples(const TString& key, size_t targetSize)
    : key_(key), targetSize_(targetSize) {}

void WFD5WaveformIntegralPresamples::AddSample(double value) {
    if (!IsFull()) {
        samples_.push_back(value);
    }
}

bool WFD5WaveformIntegralPresamples::IsFull() const {
    return samples_.size() >= targetSize_;
}

size_t WFD5WaveformIntegralPresamples::Size() const {
    return samples_.size();
}

size_t WFD5WaveformIntegralPresamples::TargetSize() const {
    return targetSize_;
}

double WFD5WaveformIntegralPresamples::Mean() const {
    if (samples_.empty()) return 0.0;
    double sum = std::accumulate(samples_.begin(), samples_.end(), 0.0);
    return sum / samples_.size();
}

double WFD5WaveformIntegralPresamples::Sigma() const {
    if (samples_.size() < 2) return 0.0;
    double mean = Mean();
    double sq_sum = 0.0;
    for (double v : samples_) {
        sq_sum += (v - mean) * (v - mean);
    }
    return std::sqrt(sq_sum / (samples_.size() - 1));
}

void WFD5WaveformIntegralPresamples::Print(Option_t* /*option*/) const {
    std::cout << String() << std::endl;
}

std::string WFD5WaveformIntegralPresamples::String() const {
    std::ostringstream oss;
    oss << "WFD5WaveformIntegralPresamples {"
        << " key=" << key_
        << ", size=" << samples_.size()
        << ", targetSize=" << targetSize_
        << ", mean=" << Mean()
        << ", sigma=" << Sigma()
        << " }";
    return oss.str();
}
