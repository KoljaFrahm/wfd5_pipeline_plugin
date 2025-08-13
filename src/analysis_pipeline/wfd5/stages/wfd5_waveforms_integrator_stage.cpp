#include "analysis_pipeline/wfd5/stages/wfd5_waveforms_integrator_stage.h"

#include <memory>
#include <spdlog/spdlog.h>
#include <TList.h>

using namespace dataProducts;

ClassImp(WFD5WaveformsIntegratorStage)

WFD5WaveformsIntegratorStage::WFD5WaveformsIntegratorStage() = default;

void WFD5WaveformsIntegratorStage::OnInit() {
    inputLabel_ = parameters_.value("input_product", "WFD5WaveformCollection");
    outputLabel_ = parameters_.value("product_name", "WaveformIntegralCollection");

    nsigma_ = parameters_.value("nsigma", 10.0);
    searchMethod_ = parameters_.value("search_method", 0);

    // presample_config expected as two-element array or tuple
    if (parameters_.contains("presample_config")) {
        auto presamples = parameters_.get<std::vector<int>>("presample_config");
        if (presamples.size() == 2) {
            presampleConfig_ = {presamples[0], presamples[1]};
        } else {
            spdlog::warn("[{}] presample_config parameter malformed, expected 2 elements", Name());
        }
    }

    seedIndex_ = parameters_.value("seed_index", -1);
    seededSearchWindow_ = parameters_.value("seeded_search_window", -1);

    spdlog::debug("[{}] Initialized with input='{}', output='{}', nsigma={}, searchMethod={}, presampleConfig=({},{}), seedIndex={}, seededSearchWindow={}",
                  Name(), inputLabel_, outputLabel_, nsigma_, searchMethod_,
                  presampleConfig_.first, presampleConfig_.second, seedIndex_, seededSearchWindow_);
}

void WFD5WaveformsIntegratorStage::Process() {
    if (!getDataProductManager()->hasProduct(inputLabel_)) {
        spdlog::warn("[{}] Input product '{}' not found", Name(), inputLabel_);
        return;
    }

    auto inputHandle = getDataProductManager()->checkoutWrite(inputLabel_);
    auto* waveformList = dynamic_cast<TList*>(inputHandle->getObject());

    if (!waveformList) {
        spdlog::error("[{}] Input '{}' is not a TList", Name(), inputLabel_);
        return;
    }

    auto outputList = std::make_unique<TList>();
    outputList->SetOwner(kTRUE);

    int count = 0;
    for (TObject* obj : *waveformList) {
        auto* waveform = dynamic_cast<WFD5Waveform*>(obj);
        if (!waveform) continue;

        // Create new WaveformIntegral object
        auto* integral = new WaveformIntegral();

        // Fill metadata from waveform
        integral->crateNum = waveform->crateNum;
        integral->amcNum = waveform->amcNum;
        integral->channelTag = waveform->channelTag;
        integral->runNum = waveform->runNum;
        integral->subRunNum = waveform->subRunNum;
        integral->waveformIndex = waveform->waveformIndex;
        integral->length = waveform->length;
        integral->pedestalLevel = waveform->pedestalLevel;
        integral->pedestalStdev = waveform->pedestalStdev;
        integral->detectorSystem = waveform->detectorSystem;
        integral->subdetector = waveform->subdetector;
        integral->eventNum = waveform->eventNum;
        integral->raw = waveform;

        // Perform integration using config params
        integral->DoIntegration(presampleConfig_, seedIndex_, seededSearchWindow_);
        integral->search_method = searchMethod_;
        integral->nsigma = nsigma_;

        outputList->Add(integral);
        ++count;
    }

    auto pdp = std::make_unique<PipelineDataProduct>();
    pdp->setName(outputLabel_);
    pdp->setObject(std::move(outputList));
    pdp->addTag("WFD5");
    pdp->addTag("waveform_integral");
    pdp->addTag("crate_amc_channel");
    pdp->addTag("integral_list");
    pdp->addTag("built_by_wfd5_waveforms_integrator");
    getDataProductManager()->addOrUpdate(outputLabel_, std::move(pdp));

    spdlog::debug("[{}] Integrated {} waveforms into WaveformIntegral objects", Name(), count);
}
