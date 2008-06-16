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

#ifndef _BLOOMFILTER_H_
#define _BLOOMFILTER_H_

#include "BloomFilterBase.h"

#include <ostream>

/* Bitmap vector for normal BloomFilter */
class Bitmap {
    friend std::ostream & operator << (std::ostream &, const Bitmap &);

    public:
	Bitmap(size_t size = 0) : bitmap(NULL)
	{
	    resize(size);
	}


	~Bitmap()
	{
	    free(bitmap);
	}

	typedef bool ValueType;

	void resize(size_t size);
	void clear();
	void set(size_t index);
	bool get(size_t index) const;

    private:
	uint8_t* bitmap;
	size_t len_bits;
	size_t len_octets;
};

std::ostream & operator << (std::ostream &, const Bitmap &);



/* BloomFilter normal bloom filter */
class BloomFilter : public BloomFilterBase<Bitmap>
{
    friend std::ostream & operator << (std::ostream &, const BloomFilter &);

    public:
	BloomFilter(HashParams* hashParams, size_t size, bool CMS = true) : BloomFilterBase<Bitmap>(hashParams, size, CMS) {}

	virtual ~BloomFilter() {}

	virtual bool get(const uint8_t* input, size_t len) const;
	virtual void set(const uint8_t* input, size_t len, bool v = true);
};

#endif // _BLOOMFILTER_H_
