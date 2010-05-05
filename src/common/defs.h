/*
 * global definitions for Vermont
 * Copyright (C) 2007 Vermont development team <http://vermont.berlios.de>
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

#ifndef DEFS_H
#define DEFS_H

/*
 * global definitions for Vermont
 */

/**
 * default sensor manager's check interval in seconds
 * (used if it was not specified in configuration)
 */
#define SM_DEFAULT_CHECK_INTERVAL 10

/**
 * default sensor manager's filename where sensor data
 * will be written to
 */
#define SM_DEFAULT_OUTPUT_FNAME "sensor_output.xml"

/**
 * default sensor manager's flag variable that determines, if
 * output file should be appended to or overwritten
 */
#define SM_DEFAULT_APPEND 0

/**
 * default aggregator polling time in milliseconds
 * how often should aggregator check hashtable for expired flows?
 */
#define AGG_DEFAULT_POLLING_TIME 10000

/**
 * defines in milliseconds, how long flow records may be cached in IpfixSender
 * until they are sent to the network
 */
#define IS_DEFAULT_RECORDCACHETIMEOUT 5000

/**
 * defines in packets per seconds, how many udp records are to be sent over
 * the wire
 */
#define IS_DEFAULT_MAXRECORDRATE 5000

/**
 * defines amount of milliseconds, how long a SCTP socket tries to retransmit
 * data
 */
#define IS_DEFAULT_SCTP_DATALIFETIME 10000

/**
 * defines interval in seconds, how often IpfixExporter tries to reestablish a
 * lost SCTP connection
 */
#define IS_DEFAULT_SCTP_RECONNECTINTERVAL 30

/**
 * defines interval in seconds, how often IpfixExporter resends an IPFIX template
 */
#define IS_DEFAULT_TEMPLATE_TIMEINTERVAL 30

/**
 * defines interval in records, how often IpfixExporter resends an IPFIX template
 */
#define IS_DEFAULT_TEMPLATE_RECORDINTERVAL 10000

/**
 * defines default size of used hashtables in bits
 */
#define HT_DEFAULT_BITSIZE 17


/**
 * defines interval in milliseconds, how often DelayedDeleter is called
 * 10 * 1000ms = 10s
 */

#define DELETER_PERIOD 10000


/**
 * After a module is put on the deleter list, we will wait at least
 * 20 seconds, before we destroy it
 */

#define DELETER_DELAY 20



/**
 * convenient way to determine size of a C array
 */
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))



#endif /*DEFS_H*/
