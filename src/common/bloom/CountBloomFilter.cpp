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

#include "CountBloomFilter.h"

void CountArray::resize(size_t size)
{
	free(array);
	array_size = size;
	array = (ValueType*)(malloc(size*sizeof(ValueType)));
	clear();
}

void CountArray::clear()
{
	memset(array, 0, array_size*sizeof(ValueType));
}

void CountArray::set(size_t index, ValueType value)
{
	if(index < array_size) {
		array[index] += value;
		//msg(MSG_DEBUG, "array[%u] == %u", index, array[index]);
	}
}

CountArray::ValueType CountArray::get(size_t index) const
{
	if(index < array_size) {
		//msg(MSG_DEBUG, "get: array[%u] == %u", index, array[index]);
		return array[index];
	} else {
		return 0;
	}
}

std::ostream & operator << (std::ostream & os, const CountArray & a) 
{
    for(size_t i=0; i<a.array_size; i++)
    {
	os << a.get(i) << " ";
    }
    return os;
}

