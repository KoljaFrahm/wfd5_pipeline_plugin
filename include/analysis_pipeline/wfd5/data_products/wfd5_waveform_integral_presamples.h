#ifndef WFD5_WAVEFORM_INTEGRAL_PRESAMPLES_HH
#define WFD5_WAVEFORM_INTEGRAL_PRESAMPLES_HH

#include <TObject.h>
#include <TString.h>
#include <vector>
#include <string>

/**
 * @class WFD5WaveformIntegralPresamples
 * @brief ROOT container for storing presamples of integrals before histogram creation.
 *
 * Stores up to N samples for one histogram key (crate/amc/channel).
 * Once full, mean and sigma can be computed to define histogram ranges.
 */
class WFD5WaveformIntegralPresamples : public TObject {
public:
    WFD5WaveformIntegralPresamples() = default;
    WFD5WaveformIntegralPresamples(const TString& key, size_t targetSize);
    ~WFD5WaveformIntegralPresamples() override = default;

    void AddSample(double value);

    bool IsFull() const;
    size_t Size() const;
    size_t TargetSize() const;

    double Mean() const;
    double Sigma() const;

    const TString& Key() const { return key_; }

    void Print(Option_t* option = "") const override;
    std::string String() const;

private:
    TString key_;
    std::vector<double> samples_;
    size_t targetSize_ = 100;

    ClassDefOverride(WFD5WaveformIntegralPresamples, 1);
};

#endif // WFD5_WAVEFORM_INTEGRAL_PRESAMPLES_HH
