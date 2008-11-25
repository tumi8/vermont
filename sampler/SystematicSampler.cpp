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

#include <sstream>

bool SystematicSampler::processPacket(Packet *p)
{
	struct timeval elapsed;
	unsigned long msecs;

	packetCount++;

	switch(samplingType) {
	case SYSTEMATIC_SAMPLER_TIME_BASED:
		// calculate time elapsed since start of sampling
		timersub(&p->timestamp, &this->startTime, &elapsed);

		// calculate number of milliseconds
		msecs = (elapsed.tv_sec * 1000) + (elapsed.tv_usec / 1000);

		// normalize to interval borders and check if it's <= onTime
		return ((msecs % interval) < samplingOnTime);
		break;

	case SYSTEMATIC_SAMPLER_COUNT_BASED:
		return ((packetCount % interval) < samplingOnTime);
		break;
	}

	return 0;
}

std::string SystematicSampler::getStatisticsXML(double interval)
{
	ostringstream oss;

	oss << "<SystematicSampler type=\"";
	switch (samplingType) {
		case SYSTEMATIC_SAMPLER_TIME_BASED:
			oss << "timeBased\">";
			break;
		case SYSTEMATIC_SAMPLER_COUNT_BASED:
			oss << "countBased\">";
			break;
	}

	oss << "<spacing>" << samplingOnTime << "</spacing>";
	oss << "<interval>" << this->interval << "</interval>";
	oss << "</SystematicSampler>";
	return oss.str();
}

