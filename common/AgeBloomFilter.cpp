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

#include "AgeBloomFilter.h"

void AgeArray::resize(size_t size)
{
	free(array);
	array_size = size;
	array = (agetime_t*)malloc(size*sizeof(agetime_t));
	clear();
}

void AgeArray::clear()
{
	memset(array, 0, array_size*sizeof(agetime_t));
}

void AgeArray::set(size_t index, agetime_t time)
{
	if(index < array_size) {
		array[index] = time;
	}
}

agetime_t AgeArray::get(size_t index) const
{
    if(index < array_size)
	return array[index];
    else
	return 0;
}

std::ostream & operator << (std::ostream & os, const AgeArray & a) 
{
    for(uint32_t i=0; i<a.array_size; i++)
    {
	os << a.get(i) << " ";
    }
    return os;
}

