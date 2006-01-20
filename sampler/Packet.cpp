/*
 * PSAMP Reference Implementation
 *
 * Packet.cpp
 *
 * Encapsulates a captured packet with simple, thread-aware
 * reference-(usage-) counting.
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

/*
 changed by: Ronny T. Lampert, 2005, for VERMONT
 */

#include "Packet.h"

/*
 this is the DEFAULT value (Ethernet specific)
 it is now a static const int defined in the header file, because this makes more sense
 const int Packet::IPHeaderOffset = 14;
 */

// keeps track on how many packets we received until now
unsigned long Packet::totalPacketsReceived = 0;

unsigned char Packet::nullBuffer[64] = { 0 };

