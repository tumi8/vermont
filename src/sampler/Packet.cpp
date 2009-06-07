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


// keeps track on how many packets we received until now
unsigned long Packet::totalPacketsReceived = 0;
