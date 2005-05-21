/*
 * PSAMP Reference Implementation
 *
 * SystematicSampler.h
 *
 * Systematic sampling of packets (count-based/time-based)
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#ifndef SYSTEMATIC_SAMPLER_H
#define SYSTEMATIC_SAMPLER_H

#define SYSTEMATIC_SAMPLER_COUNT_BASED 1
#define SYSTEMATIC_SAMPLER_TIME_BASED 2

#include <sys/time.h>
#include "PacketProcessor.h"

class SystematicSampler : public PacketProcessor
{
public:

        /*
         constructs a new systematic sampler (see section 3.1.1 of PSAMP-Sample-Tech)
         if type is CountBasedSampler then onTime and offTime specify how long to sample.
         for example if onTime = 10 and offTime = 90, we sample 10 packets out of 100
         if type is TimeBasedSampler, then onTime and offTime are specified in milliseconds
         and specify how long to keep capturing packets.
         */
        SystematicSampler(int type, unsigned long onTime, unsigned long offTime) :
                samplingType(type), samplingOnTime(onTime), samplingOffTime(offTime), packetCount(0)
        {
                gettimeofday(&startTime, 0);

                interval = samplingOnTime + samplingOffTime;
        };

        ~SystematicSampler();

        virtual bool processPacket(const Packet *p);

protected:
        int samplingType;

        unsigned long samplingOnTime;
        unsigned long samplingOffTime;
        unsigned long interval;

        // for time-based sampler:
        struct timeval startTime;

        // for count-based samplers
        unsigned long packetCount;
};

#endif
