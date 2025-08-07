#include "analysis_pipeline/wfd5/data_products/hodoscope_event.h"

#include <iostream>
#include <sstream>

using namespace dataProducts;

ClassImp(HodoscopeEvent)

HodoscopeEvent::HodoscopeEvent()
    : amplitude_x(0.), integral_x(0.), fullintegral_x(0.),
      amplitude_y(0.), integral_y(0.), fullintegral_y(0.),
      x(0.), y(0.), max_x(0.), max_y(0.),
      max_channel_x(0), max_amc_x(0), max_crate_x(0), max_integral_x(-1e12),
      max_channel_y(0), max_amc_y(0), max_crate_y(0), max_integral_y(-1e12),
      nx(0), ny(0) {}

void HodoscopeEvent::Print(Option_t* /*option*/) const {
    std::cout << String() << std::endl;
}

std::string HodoscopeEvent::String() const {
    std::ostringstream oss;
    oss << "HodoscopeEvent {"
        << " amplitude_x = " << amplitude_x
        << ", integral_x = " << integral_x
        << ", fullintegral_x = " << fullintegral_x
        << ", amplitude_y = " << amplitude_y
        << ", integral_y = " << integral_y
        << ", fullintegral_y = " << fullintegral_y
        << ", x = " << x
        << ", y = " << y
        << ", max_x = " << max_x
        << ", max_y = " << max_y
        << ", max_channel_x = " << max_channel_x
        << ", max_amc_x = " << max_amc_x
        << ", max_crate_x = " << max_crate_x
        << ", max_integral_x = " << max_integral_x
        << ", max_channel_y = " << max_channel_y
        << ", max_amc_y = " << max_amc_y
        << ", max_crate_y = " << max_crate_y
        << ", max_integral_y = " << max_integral_y
        << ", nx = " << nx
        << ", ny = " << ny
        << " }";
    return oss.str();
}
