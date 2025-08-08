// wfd5_hodoscope_position_histogram_stage.h
#ifndef WFD5_PIPELINE_PLUGIN_STAGES_WFD5_HODOSCOPE_POSITION_HISTOGRAM_STAGE_H
#define WFD5_PIPELINE_PLUGIN_STAGES_WFD5_HODOSCOPE_POSITION_HISTOGRAM_STAGE_H

#include "analysis_pipeline/core/stages/base_stage.h"
#include "analysis_pipeline/wfd5/data_products/hodoscope_event.h"
#include <string>
#include <vector>
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

    int binsX_ = 0;
    int binsY_ = 0;
    int xMinInt_ = 0;
    int xMaxInt_ = 0;
    int yMinInt_ = 0;
    int yMaxInt_ = 0;

    void FillHistogram(TH2D* hist, const HodoscopeEvent* evt);
    void BuildIntegerBinEdges(std::vector<double>& edges, int minVal, int maxVal);

    ClassDefOverride(WFD5HodoscopePositionHistogramStage, 1);
};

#endif // WFD5_PIPELINE_PLUGIN_STAGES_WFD5_HODOSCOPE_POSITION_HISTOGRAM_STAGE_H
