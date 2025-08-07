#ifndef WFD5_PIPELINE_PLUGIN_STAGES_WFD5_HODOSCOPE_POSITION_HISTOGRAM_STAGE_H
#define WFD5_PIPELINE_PLUGIN_STAGES_WFD5_HODOSCOPE_POSITION_HISTOGRAM_STAGE_H

#include "analysis_pipeline/core/stages/base_stage.h"
#include "analysis_pipeline/wfd5/data_products/hodoscope_event.h"
#include <string>
#include <TH2D.h>

using namespace dataProducts;

class WFD5HodoscopePositionHistogramStage : public BaseStage {
public:
    WFD5HodoscopePositionHistogramStage() = default;
    ~WFD5HodoscopePositionHistogramStage() override = default;

    void OnInit() override;
    void Process() override;

    std::string Name() const override { return "WFD5HodoscopePositionHistogramStage"; }

private:
    std::string inputLabel_;
    std::string outputLabel_;
    std::string title_;
    int binsX_ = 100;
    int binsY_ = 100;
    double xMin_ = 0;
    double xMax_ = 100;
    double yMin_ = 0;
    double yMax_ = 100;

    void FillHistogram(TH2D* hist, const class HodoscopeEvent* evt);

    ClassDefOverride(WFD5HodoscopePositionHistogramStage, 1);
};

#endif // WFD5_PIPELINE_PLUGIN_STAGES_WFD5_HODOSCOPE_POSITION_HISTOGRAM_STAGE_H
