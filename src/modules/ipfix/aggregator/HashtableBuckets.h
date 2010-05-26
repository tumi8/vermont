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


#ifndef BUCKETLIST_H_
#define BUCKETLIST_H_


class BucketListElement;

/**
 * Single Bucket containing one buffered flow's variable data.
 *Is either a direct entry in @c Hashtable::bucket or a member of another Hashtable::Bucket's spillchain
 */
class HashtableBucket
{
public:
	uint32_t expireTime; /**<timestamp when this bucket will expire if no new flows are added*/
	uint32_t forceExpireTime; /**<timestamp when this bucket is forced to expire */
	boost::shared_array<IpfixRecord::Data> data; /**< contains variable fields of aggregated flow; format defined in Hashtable::dataInfo::fieldInfo*/
	bool forceExpiry; /**< is set to true when bucket must be exported immediately */
	bool inTable; /**< set to true when bucket is listed in the hashtable */
	HashtableBucket* prev; /**< previous bucket in spillchain */
	HashtableBucket* next; /**< next bucket in spillchain */
	uint32_t observationDomainID;
	BucketListElement* listNode;
	uint32_t hash;
};


class BucketListElement : public ManagedInstance<BucketListElement>
{
public:
	BucketListElement* next; //next Element in list
	BucketListElement* prev; //previous Element in list
	HashtableBucket* bucket;


	BucketListElement(InstanceManager<BucketListElement>* im)
		: ManagedInstance<BucketListElement>(im),
		next(0),
		prev(0),
		bucket(0)
	{
	}

	void reset()
	{
		next = 0;
		prev = 0;
		bucket = 0;
	}
};


class HashtableBucketList
{
public:
	BucketListElement* head;
	BucketListElement* tail;
	bool isEmpty;

	HashtableBucketList()
		: head(0), tail(0), isEmpty(true)
	{
	}


	/**
	 * adds the given Element at the end of the list
	 */
	void push(BucketListElement* node)
	{
		if (isEmpty) {
			isEmpty = false;
			head = node;
			tail = head;
			node->prev = 0;
			node->next = 0;
		} else {
			tail->next = node;
			node->prev = tail;
			node->next = 0;
			tail = node;
		}
	}

	/**
	 * removes the given Element from the list
	 */
	void remove(BucketListElement* node)
	{
		if (isEmpty)
			THROWEXCEPTION("List is empty: can't remove Node");
		else {
			if (!node->prev) {
				if (!node->next) {
					isEmpty = 1;
					head = 0;
					tail = 0;
				} else {
					node->next->prev = 0;
					head = node->next;
				}
			} else if (!node->next) {
				node->prev->next = 0;
				tail = node->prev;
			} else {
				node->prev->next = node->next;
				node->next->prev = node->prev;
			}
		}
	}

};

#endif /*BUCKETLIST_H_*/
