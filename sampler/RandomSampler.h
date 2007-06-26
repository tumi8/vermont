/*
 * PSAMP Reference Implementation
 *
 * RandomSampler.h
 *
 * Random n-out-of-N sampling of packets
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

// implementation of the sampler explained in section 3.1.2.1 of PSAMP-Sample-Tech

#ifndef RANDOM_SAMPLER_H
#define RANDOM_SAMPLER_H

#include <vector>

#include "common/msg.h"
#include "PacketProcessor.h"

class RandomSampler : public PacketProcessor
{
public:
        RandomSampler(int n, int N);
	virtual ~RandomSampler()
	{
	};
	
        virtual bool processPacket(const Packet *p);

protected:
        /* N */
        int samplingSize;
        /* n */
        int acceptSize;
        int currentPos;
        std::vector<bool> sampleMask;
};

#endif
