#ifndef _MIN_BLOOMFILTER_
#define _MIN_BLOOMFILTER_

#include "BloomFilterBase.h"

#include "msg.h"

#include <climits>

// work around a gcc "feature": whenever you need to access a variable from BloomFilterBase<T>,
// you need to specify the full namespace, e.g.  BloomFilterBase::hf_numbers

template <class T>
class MinBloomFilter : public BloomFilterBase<T>
{
	public:
		MinBloomFilter(HashParams* hashParams, size_t filterSize, bool CMS = true)
			: BloomFilterBase<T>(hashParams, filterSize, CMS) {}

		virtual ~MinBloomFilter() {}


		virtual typename T::ValueType  get(const uint8_t* input, size_t len) const {
			typename T::ValueType  ret = INT_MAX;
			typename T::ValueType  current;
			for(unsigned i=0; i != BloomFilterBase<T>::hfList->len; i++) {   
				if (BloomFilterBase<T>::CMS_) {
					current = BloomFilterBase<T>::filter_[0].get(
						BloomFilterBase<T>::hashU(input, len, 
							BloomFilterBase<T>::filterSize(), BloomFilterBase<T>::hfList->seed[i]));
				} else {
					current = BloomFilterBase<T>::filter_[i].get(
						BloomFilterBase<T>::hashU(input, len,
							BloomFilterBase<T>::filterSize(), BloomFilterBase<T>::hfList->seed[i]));
				}
				if (current < ret)
					ret = current;
			}
			//msg(MSG_DEBUG, "MinBloomFilter.get(): %i", ret);
			return ret;
		}

		virtual void set(const uint8_t* input, size_t len, typename T::ValueType v) {
			//msg(MSG_DEBUG, "MinBloomFilter.set(): %i", v);
			for(unsigned i=0; i != BloomFilterBase<T>::hfList->len; i++) {
				if (BloomFilterBase<T>::CMS_) {
					BloomFilterBase<T>::filter_[0].set(BloomFilterBase<T>::hashU(input, len,
						BloomFilterBase<T>::filterSize(), 
						BloomFilterBase<T>::hfList->seed[i]), v);
				} else {
					BloomFilterBase<T>::filter_[i].set(BloomFilterBase<T>::hashU(input, len,
						BloomFilterBase<T>::filterSize(), 
						BloomFilterBase<T>::hfList->seed[i]), v);
				}
			}
		}
};

#endif
