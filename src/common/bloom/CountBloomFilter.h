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
