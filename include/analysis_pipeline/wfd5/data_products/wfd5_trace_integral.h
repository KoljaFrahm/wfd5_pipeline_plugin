#ifndef WFD5_PIPELINE_PLUGIN_DATA_PRODUCTS_WFD5_TRACE_INTEGRAL_H
#define WFD5_PIPELINE_PLUGIN_DATA_PRODUCTS_WFD5_TRACE_INTEGRAL_H

#include <cstdint>
#include <string>
#include <TObject.h>

namespace dataProducts {

class WFD5TraceIntegral : public TObject {
public:
    WFD5TraceIntegral() = default;
    WFD5TraceIntegral(int crate, int amc, uint64_t channel, double integral);

    ~WFD5TraceIntegral() override = default;

    void Print(Option_t* option = "") const override;
    std::string String() const;

    int crateNum = 0;
    int amcNum = 0;
    uint64_t channelNum = 0;
    double integralValue = 0.0;

    ClassDefOverride(WFD5TraceIntegral, 1);
};

} // namespace dataProducts

#endif // WFD5_PIPELINE_PLUGIN_DATA_PRODUCTS_WFD5_TRACE_INTEGRAL_H
