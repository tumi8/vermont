/*
 * VERMONT
 * Copyright (C) 2010 Tobias Limmer <limmer@cs.fau.de>
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
 */

#if !defined(IDSLOADSTATISTICS_H__)
#define IDSLOADSTATISTICS_H__


struct IDSLoadStatistics
{
	uint32_t sysJiffies;
	uint32_t userJiffies;
	uint64_t droppedPackets;
	uint64_t droppedOctets;
	bool running;

	IDSLoadStatistics() {}
	IDSLoadStatistics(bool r, uint64_t dp, uint64_t doc, uint32_t sj=0, uint32_t uj=0)
		: sysJiffies(sj),
		  userJiffies(uj),
		  droppedPackets(dp),
		  droppedOctets(doc),
		  running(r)
	{}
};

#endif
