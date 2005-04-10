/*
 * PSAMP Reference Implementation
 *
 * SystematicSampler.cpp
 *
 * Implementation of systematic sampler
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#include "SystematicSampler.h"

bool SystematicSampler::processPacket(const Packet *p)
{
  struct timeval elapsed;
  unsigned long msecs;
  
  // calculate time elapsed since start of sampling
  timersub(&p->timestamp, &this->startTime, &elapsed);
  
  // calculate number of milliseconds
  msecs = (elapsed.tv_sec * 1000) + (elapsed.tv_usec / 1000);
    
  packetCount++;
  
  if (samplingType == SystematicSampler::TimeBasedSampler)
  {
    // normalize to interval borders and check if it's <= onTime
    return ((msecs % interval) < samplingOnTime);
  }
  else
  {
    return ((packetCount % interval) < samplingOnTime);
  }
}
