#
# VERMONT build scripts for CMake
# Copyright (C) 2007 Peter Baumann <siprbaum@users.berlios.de>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

ADD_LIBRARY(cfg
	AggregatorBaseCfg.cpp
	Cfg.cpp
	CfgNode.cpp
	ConfigManager.cpp
	Connector.cpp
	DelayedDeleter.cpp
	Graph.cpp
	GraphInstanceSupplier.cpp
	IDMEFExporterCfg.cpp
	IpfixAggregatorCfg.cpp
	IpfixCollectorCfg.cpp
	IpfixDbReaderCfg.cpp
	IpfixDbWriterCfg.cpp
	IpfixDbWriterPgCfg.cpp
	IpfixExporterCfg.cpp
	IpfixPayloadWriterCfg.cpp
	IpfixPrinterCfg.cpp
	IpfixSamplerCfg.cpp
	Node.cpp
	ObserverCfg.cpp
	PacketAggregatorCfg.cpp
	PacketFilterCfg.cpp
	PacketIDMEFReporterCfg.cpp
	PacketReportingCfg.cpp
	PSAMPExporterCfg.cpp
	PCAPExporterCfg.cpp
	ReConnector.cpp
	RecordAnonymizerCfg.cpp
	AutoFocusCfg.cpp
	RBSWormDetectorCfg.cpp
	SensorManager.cpp
	SensorManagerCfg.cpp
	TRWPortscanDetectorCfg.cpp
	P2PDetectorCfg.cpp
	XMLAttribute.cpp
	XMLElement.cpp
	XMLDocument.cpp
	XMLNode.cpp
	XMLTextNode.cpp
)

