#ifndef HODOSCOPE_EVENT_HH
#define HODOSCOPE_EVENT_HH

#include <string>
#include <TObject.h>

namespace dataProducts {

class HodoscopeEvent : public TObject {
public:
    HodoscopeEvent() = default;
    ~HodoscopeEvent() override = default;

    void Print(Option_t* option = "") const override;
    std::string String() const;

    // Properties to be calculated
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


    ClassDefOverride(HodoscopeEvent, 1)
};

}  // namespace dataProducts

#endif  // HODOSCOPE_EVENT_HH
