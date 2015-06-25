ADD_LIBRARY(core
	Cfg.cpp
	CfgNode.cpp
	Connector.cpp
	Graph.cpp
	GraphInstanceSupplier.cpp
	MainSignalHandler.cpp
	Module.cpp
	SensorManager.cpp
	Node.cpp
	XMLAttribute.cpp
	XMLElement.cpp
	XMLDocument.cpp
	XMLNode.cpp
	XMLTextNode.cpp
)

add_cppcheck(core STYLE POSSIBLE_ERROR)
