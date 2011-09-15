/*
 * Vermont Aggregator Subsystem
 * Copyright (C) 2009 Vermont Project
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
#ifdef PLUGIN_SUPPORT_ENABLED
#ifndef PACKETFUNCTIONS_H_
#define PACKETFUNCTIONS_H_

/**
* Methods taken from the OSLRd project
*
*/
static inline void
pkt_get_u8(const uint8_t ** p, uint8_t * var)
{
    *var = *(const uint8_t *)(*p);
    *p += sizeof(uint8_t);
}
static inline void
pkt_get_u16(const uint8_t ** p, uint16_t * var)
{
    *var = ntohs(**((const uint16_t **)p));
    *p += sizeof(uint16_t);
}
static inline void
pkt_get_u32(const uint8_t ** p, uint32_t * var)
{
    *var = ntohl(**((const uint32_t **)p));
    *p += sizeof(uint32_t);
}
static inline void
pkt_get_s8(const uint8_t ** p, int8_t * var)
{
    *var = *(const int8_t *)(*p);
    *p += sizeof(int8_t);
}
static inline void
pkt_get_s16(const uint8_t ** p, int16_t * var)
{
    *var = ntohs(**((const int16_t **)p));
    *p += sizeof(int16_t);
}
static inline void
pkt_get_s32(const uint8_t ** p, int32_t * var)
{
    *var = ntohl(**((const int32_t **)p));
    *p += sizeof(int32_t);
}

static inline void
pkt_ignore_u8(const uint8_t ** p)
{
    *p += sizeof(uint8_t);
}
static inline void
pkt_ignore_u16(const uint8_t ** p)
{
    *p += sizeof(uint16_t);
}
static inline void
pkt_ignore_u32(const uint8_t ** p)
{
    *p += sizeof(uint32_t);
}
static inline void
pkt_ignore_s8(const uint8_t ** p)
{
    *p += sizeof(int8_t);
}
static inline void
pkt_ignore_s16(const uint8_t ** p)
{
    *p += sizeof(int16_t);
}
static inline void
pkt_ignore_s32(const uint8_t ** p)
{
    *p += sizeof(int32_t);
}
#endif
#endif
