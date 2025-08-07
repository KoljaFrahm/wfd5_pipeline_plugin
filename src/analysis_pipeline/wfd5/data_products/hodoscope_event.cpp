#include "analysis_pipeline/wfd5/data_products/hodoscope_event.h"

#include <iostream>
#include <sstream>

using namespace dataProducts;

ClassImp(HodoscopeEvent)

void HodoscopeEvent::Print(Option_t* /*option*/) const {
    std::cout << String() << std::endl;
}

std::string HodoscopeEvent::String() const {
    std::ostringstream oss;
    oss << "HodoscopeEvent {"
        << " max_x=" << max_x
        << ", max_y=" << max_y
        << ", max_channel_x=" << max_channel_x
        << ", max_amc_x=" << max_amc_x
        << ", max_crate_x=" << max_crate_x
        << ", max_integral_x=" << max_integral_x
        << ", max_channel_y=" << max_channel_y
        << ", max_amc_y=" << max_amc_y
        << ", max_crate_y=" << max_crate_y
        << ", max_integral_y=" << max_integral_y
        << " }";
    return oss.str();
}
