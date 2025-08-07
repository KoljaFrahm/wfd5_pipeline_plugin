#ifdef __CINT__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

// Data product classes (namespace dataProducts)
#pragma link C++ class dataProducts::WFD5TraceIntegral+;
#pragma link C++ class dataProducts::HodoscopeEvent+;

// Pipeline stages (global namespace)
#pragma link C++ class WFD5WaveformsIntegratorStage+;
#pragma link C++ class WFD5TraceIntegralHistogramStage+;
#pragma link C++ class WFD5HodoscopeEventBuilderStage+;
#pragma link C++ class WFD5HodoscopePositionHistogramStage+;
#pragma link C++ class WFD5ChannelMappingStage+;
#pragma link C++ class WFD5PedestalCorrectionStage+;

#endif
