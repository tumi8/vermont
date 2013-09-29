/*
 * IPFIX Database Reader/Writer
 * Copyright (C) 2006 Jürgen Abberger
 * Copyright (C) 2006 Lothar Braun <braunl@informatik.uni-tuebingen.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

/* Some constants that are common to IpfixDbWriter and IpfixDbReader */
/**
 * STARTLEN     : Length of statement for INSERT IN.., CREATE TABL.., CREATE DATA.., SELECT FROM...
 * TABLE_WIDTH  : Length of table name string
 * COL_WIDTH    : Length of the string denotes the name of the single columns
 *                and datatype to store in table
 * INS_WIDTH    : Length of the string for insert statement depending of count columns
 * EXPORTER_WIDTH    : Length of the string for operations on exporter table
 */

#if defined(DB_SUPPORT_ENABLED) || defined(MONGO_SUPPORT_ENABLED) || defined(PG_SUPPORT_ENABLED) || defined(ORACLE_SUPPORT_ENABLED) || defined(REDIS_SUPPORT_ENABLED)

#include "IpfixDbWriterSQL.hpp"

#define STARTLEN         60
#define TABLE_WIDTH      16
#define COL_WIDTH        40
#define INS_WIDTH        25
#define EXPORTER_WIDTH   80

// column names
#define CN_srcIP 	"srcIP"
#define CN_dstIP 	"dstIP"
#define CN_srcPort 	"srcPort"
#define CN_dstPort 	"dstPort"
#define CN_proto	"proto"
#define CN_dstTos       "dstTos"
#define CN_bytes       	"bytes"
#define CN_pkts       	"pkts"
#define CN_firstSwitched 	"firstSwitched"
#define CN_lastSwitched 	"lastSwitched"
#define CN_firstSwitchedMillis 	"firstSwitchedMillis"
#define CN_lastSwitchedMillis 	"lastSwitchedMillis"
#define CN_exporterID 	"exporterID"

#define CN_tcpControlBits	"tcpControlBits"
#define CN_revbytes		"revbytes"
#define CN_revpkts		"revpkts"
#define CN_revFirstSwitched	"revFirstSwitched"
#define CN_revLastSwitched	"revLastSwitched"
#define CN_revFirstSwitchedMillis	"revFirstSwitchedMillis"
#define CN_revLastSwitchedMillis	"revLastSwitchedMillis"
#define CN_revTcpControlBits	"revTcpControlBits"
#define CN_maxPacketGap		"maxPacketGap"
#define CN_revMaxPacketGap      "revMaxPacketGap"

#define CN_flowStartSysUpTime	"flowStartSysUpTime"
#define CN_flowEndSysUpTime	"flowEndSysUpTime"


#endif
