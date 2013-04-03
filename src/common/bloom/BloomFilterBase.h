/**************************************************************************/
/*    Copyright (C) 2008 Lothar Braun                                     */
/*                                                                        */
/*    This library is free software; you can redistribute it and/or       */
/*    modify it under the terms of the GNU Lesser General Public          */
/*    License as published by the Free Software Foundation; either        */
/*    version 2.1 of the License, or (at your option) any later version.  */
/*                                                                        */
/*    This library is distributed in the hope that it will be useful,     */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of      */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   */
/*    Lesser General Public License for more details.                     */
/*                                                                        */
/*    You should have received a copy of the GNU Lesser General Public    */
/*    License along with this library; if not, write to the Free Software  */
/*    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA    */
/**************************************************************************/

#ifndef _BLOOMFILTER_BASE_H_
#define _BLOOMFILTER_BASE_H_

#ifdef HAVE_GSL
#include <gsl/gsl_rng.h>
#endif

#include <stdint.h>
#include <cstring>
#include <ctime>

#include <modules/packet/Packet.h>

/* GenericKey class holding uint8_t* input for BloomFilter hash functions */
template<unsigned size> class GenericKey
{
	public:
		unsigned len;
		uint8_t data[size];

		GenericKey() : len(0) {
			memset(data,0,sizeof(data));
		}

		void set(uint8_t *input, unsigned inputlen)
		{
			len = inputlen<sizeof(data)?inputlen:sizeof(data);
			memcpy(&data, input, len);
		}

		void reset()
		{
			len = 0;
			memset(data,0,sizeof(data));
		}

		void append(uint8_t *input, unsigned inputlen)
		{
			if(len<sizeof(data)) {
				if((len+inputlen) < sizeof(data)) {
					memcpy(&(data[len]), input, inputlen);
					len = len + inputlen;
				} else {
					memcpy(&(data[len]), input, sizeof(data)-len);
					len = sizeof(data);
				}
			}
		}

		bool operator<(const GenericKey& other) const 
		{
			if(len < other.len)
				return true;
			else if(len > other.len)
				return false;
			else
				return memcmp(data, other.data, len)<0?true:false;
		}
};


/* QuintupleKey class holding input for BloomFilter hash functions */
class QuintupleKey
{
	public:
		uint8_t data[15];
		const unsigned len;

		struct Quintuple
		{
			uint32_t srcIp;
			uint32_t dstIp;
			uint8_t proto;
			uint16_t srcPort;
			uint16_t dstPort;
		};

		QuintupleKey() : len(15)
		{
			memset(data,0,sizeof(data));
		}

		QuintupleKey(const Packet* p) : len(15) 
		{
			memset(data,0, sizeof(data));
			uint32_t ip1, ip2;
			uint16_t port1, port2;
			if (p->ipProtocolType == Packet::TCP) {
				ip1 = *((uint32_t*)(p->data.netHeader + 12));
				ip2 = *((uint32_t*)(p->data.netHeader + 16));
				port1 = *((uint16_t*)(p->transportHeader));
				port2 = *((uint16_t*)(p->transportHeader + 2));

				if (ip1 == ip2) {
					getQuintuple()->srcIp = getQuintuple()->dstIp = ip1;
					getQuintuple()->srcPort = port1>port2?port1:port2;
					getQuintuple()->srcPort = port1>port2?port2:port1;
				} else if (ip1 > ip2) {
					getQuintuple()->srcIp = ip1;
					getQuintuple()->dstIp = ip2;
					getQuintuple()->srcPort = port1;
					getQuintuple()->dstPort = port2;
				} else {
					getQuintuple()->srcIp = ip2;
					getQuintuple()->dstIp = ip1;
					getQuintuple()->srcPort = port2;
					getQuintuple()->dstPort = port1;
				}
			}
		}

		inline Quintuple* getQuintuple()
		{
			return (Quintuple*)data;
		}

		void reset()
		{
			memset(data,0,sizeof(data));
		}

		bool operator<(const QuintupleKey& other) const 
		{
			return memcmp(data, other.data, len)<0?true:false;
		}
};

struct HashParams
{
	HashParams(size_t l, unsigned startSeed = time(0)) : len(l) {
		seed = (uint32_t*)calloc(sizeof(uint32_t), len);
		srand(startSeed);
		for(unsigned i=0; i < len; i++) {
			seed[i] = rand();
		}
	}

	~HashParams() {
		free(seed);
	}
	size_t len;
	uint32_t* seed;
};

/**
 * BloomFilterBase provides hash functions for filters and is the interface for all type of 
 * BloomFilters. The class needs a template parameter which has to provide the following
 * methods/types:
 *
 * class MyT {
 * public:
 * 	MyT(size_t size);
 *	typedef ValueType myDesiredType;
 *	void clear();
 *	myDesiredType get(uint8_t* data, size_t len);
 *	void set(uint8_t* data, size_t len, myDesiredType value);
 * }
 */
template <class T>
class BloomFilterBase
{
	public:
		BloomFilterBase(HashParams* hashParams, unsigned filterSize, bool CMS = true) : hfList(hashParams),
			filterSize_(filterSize), CMS_(CMS)
		{
#ifdef HAVE_GSL
			r = gsl_rng_alloc (gsl_rng_fishman18);
#endif
			if (CMS_) {
				filterCount_ = 1;
			} else {
				filterCount_ = hashParams->len;
			}

			filter_ = new T[filterCount_];
			for (unsigned i = 0; i != filterCount_; ++i) {
				filter_[i].resize(filterSize_);
			}
		
			clear();
		}

		virtual ~BloomFilterBase()
		{
#ifdef HAVE_GSL
			gsl_rng_free(r);
#endif
			delete[] filter_;
		}

		virtual typename T::ValueType get(const uint8_t* data, size_t len) const = 0;
		virtual void set(const uint8_t* data, size_t len, typename T::ValueType) = 0;

		size_t filterSize() const {
			return filterSize_;
		}


	protected:

#ifdef HAVE_GSL
		uint32_t hashU(const uint8_t* input, uint16_t len, uint32_t max, uint32_t seed) const
		{
			uint32_t result = 0;
			gsl_rng_set(r, seed);
			for (unsigned i = 0; i < len; i++) {
				result += input[i] * gsl_rng_get(r);
			}
			return result % max;
		}
#else
		uint32_t hashU(const uint8_t* input, uint16_t len, uint32_t max, uint32_t seed) const
		{
			// TODO: create a function that does not need gsl
			return 0;
		}
#endif

		int32_t ggT(uint32_t m, uint32_t n)
		{
			uint32_t z;
			while (n>0) {
				z=n;
				n=m%n;
				m=z;
			}
			return m;
		}

		void clear()
		{
			for (unsigned i = 0; i != filterCount_; ++i) {
				filter_[i].clear();
			}
		}

#ifdef HAVE_GSL
		gsl_rng * r;
#endif

		const HashParams* hfList;

		size_t filterSize_;
		T* filter_;
		size_t filterCount_;
		bool CMS_;
};

#endif
