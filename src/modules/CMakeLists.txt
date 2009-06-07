ADD_LIBRARY(modules
	ConfigManager.cpp
	idmef/IDMEFExporter.cpp
	idmef/IDMEFExporterCfg.cpp
	idmef/IDMEFMessage.cpp
	idmef/PacketIDMEFReporter.cpp
	idmef/PacketIDMEFReporterCfg.cpp

	analysis/AutoFocus.cpp
	analysis/autofocus_report.cpp
	analysis/autofocus_attribute.cpp
	analysis/autofocus_iprecord.h
	analysis/TRWPortscanDetector.cpp
	analysis/RBSWormDetector.cpp
	analysis/FrontPayloadSigMatcher.cpp
	# support files for FrontPayloadSigMatcher
	analysis/fpsigmatcher/KTree.c
	analysis/fpsigmatcher/Matcher.c
	analysis/fpsigmatcher/Signature.c
	analysis/fpsigmatcher/Utils.c
	analysis/FrontPayloadSigMatcherCfg.cpp
	analysis/AutoFocusCfg.cpp
	analysis/RBSWormDetectorCfg.cpp
	analysis/TRWPortscanDetectorCfg.cpp

	packet/Observer.cpp
	packet/ObserverCfg.cpp
	packet/Packet.cpp
	packet/Template.cpp
	packet/PCAPExporterModule.cpp
	packet/PSAMPExporterModule.cpp
	packet/PSAMPExporterCfg.cpp
	packet/PCAPExporterCfg.cpp
	packet/PacketReportingCfg.cpp
	packet/filter/FilterModule.cpp
	packet/filter/PacketFilterCfg.cpp
	packet/filter/IPHeaderFilter.cpp
	packet/filter/RandomSampler.cpp
	packet/filter/RegExFilter.cpp
	packet/filter/StringFilter.cpp
	packet/filter/SystematicSampler.cpp
	packet/filter/ConnectionFilter.cpp
	packet/filter/StateConnectionFilter.cpp
	packet/filter/AnonFilter.cpp
	packet/filter/PayloadFilter.cpp
)

