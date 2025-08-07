#ifndef HODOSCOPE_EVENT_HH
#define HODOSCOPE_EVENT_HH

#include <string>
#include <TObject.h>

namespace dataProducts {

class HodoscopeEvent : public TObject {
public:
    HodoscopeEvent();
    ~HodoscopeEvent() override = default;

    void Print(Option_t* option = "") const override;
    std::string String() const;

    // Properties to be summed
    double amplitude_x = 0.0;
    double integral_x = 0.0;
    double fullintegral_x = 0.0;
    double amplitude_y = 0.0;
    double integral_y = 0.0;
    double fullintegral_y = 0.0;

    // Properties to be calculated
    double x = 0.0;
    double y = 0.0;
    double max_x = 0.0;
    double max_y = 0.0;

    double max_channel_x = 0.0;
    int max_amc_x = 0;
    int max_crate_x = 0;
    double max_integral_x = 0.0;

    double max_channel_y = 0.0;
    int max_amc_y = 0;
    int max_crate_y = 0;
    double max_integral_y = 0.0;

    int nx = 0;
    int ny = 0;

    ClassDefOverride(HodoscopeEvent, 1)
};

} // namespace dataProducts

#endif // HODOSCOPE_EVENT_HH
