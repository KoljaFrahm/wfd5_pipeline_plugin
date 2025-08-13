#include "analysis_pipeline/wfd5/data_products/wfd5_trace_integral.h"
#include <iostream>
#include <sstream>

using namespace dataProducts;

ClassImp(WFD5TraceIntegral)

WFD5TraceIntegral::WFD5TraceIntegral(int crate, int amc, uint64_t channel, double integral,
                                     const std::string& detectorSystem, const std::string& subdetector)
    : crateNum(crate),
      amcNum(amc),
      channelNum(channel),
      integralValue(integral),
      detectorSystem(detectorSystem),
      subdetector(subdetector)
{}

void WFD5TraceIntegral::Print(Option_t* /*option*/) const {
    std::cout << String() << std::endl;
}

std::string WFD5TraceIntegral::String() const {
    std::ostringstream oss;
    oss << "WFD5TraceIntegral { crate = " << crateNum
        << ", amc = " << amcNum
        << ", channel = " << channelNum
        << ", integral = " << integralValue
        << ", detectorSystem = \"" << detectorSystem << "\""
        << ", subdetector = \"" << subdetector << "\""
        << " }";
    return oss.str();
}
