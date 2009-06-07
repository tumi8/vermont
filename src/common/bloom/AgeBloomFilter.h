/**************************************************************************/
/*    Copyright (C) 2007 Gerhard Muenz                                    */
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

#ifndef _AGE_BLOOMFILTER_H_
#define _AGE_BLOOMFILTER_H_

#include "MinBloomFilter.h"

#include <ostream>

typedef time_t agetime_t;

class AgeArray {
	friend std::ostream & operator << (std::ostream &, const AgeArray &);

	public:
		AgeArray(size_t size = 0) : array(NULL)
		{
			resize(size);
		}
	
		~AgeArray()
		{
			free(array);
		}

		typedef agetime_t ValueType;

		void resize(size_t size);
		void clear();
		void set(size_t index, agetime_t time);
		agetime_t get(size_t index) const;
    
	private:
		agetime_t* array;
		size_t array_size;
};

std::ostream & operator << (std::ostream &, const AgeArray &);


typedef MinBloomFilter<AgeArray> AgeBloomFilter;

#endif // _AGE_BLOOMFILTER_H_
