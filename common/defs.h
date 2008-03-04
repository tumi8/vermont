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
 * default PCAP packet capture length in Observer module
 * if not specified in configuration file 
 */
#define PCAP_DEFAULT_CAPTURE_LENGTH 128

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
 * convenient way to determine size of a C array
 */
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))


// workaround for missing gcc functionality in non-linux environments
#if !defined(__linux__)
#define __sync_lock_test_and_set(...) 0
#define __sync_lock_release(...)
#endif

#endif /*DEFS_H*/
