#ifndef _COUNT_BLOOMFILTER_H_
#define _COUNT_BLOOMFILTER_H_

#include "MinBloomFilter.h"


#include <ostream>

class CountArray {
	public:
		CountArray(size_t size = 0) : array(NULL) {
			resize(size);
		}

		~CountArray() {
			free(array);
		}

		typedef int ValueType;

		void resize(size_t size);
		void clear();
		void set(size_t index, ValueType value);
		ValueType get(size_t index) const;

	private:
		ValueType* array;
		size_t array_size;
		friend std::ostream & operator << (std::ostream &, const CountArray &);
};

typedef MinBloomFilter<CountArray> CountBloomFilter;

#endif
