#ifndef PACKETIDMEFREPORTER_H_
#define PACKETIDMEFREPORTER_H_


#include "modules/idmef/IDMEFMessage.h"

#include "core/Destination.h"
#include "modules/packet/Packet.h"

#include <string>

using namespace std;


class PacketIDMEFReporter
	: public Module, 
	  public Destination<Packet*>,
	  public Source<IDMEFMessage*>
{
public:
	static const char* PAR_SRCIP;
	static const char* PAR_DSTIP;
	static const char* PAR_PROTOCOL;
	static const char* PAR_LENGTH;
	static const char* PAR_SRCPORT;
	static const char* PAR_DSTPORT;
	static const char* PAR_SNAPSHOT;


	PacketIDMEFReporter(string idmeftemplate, string analyzerid, uint16_t snapshotoff, uint16_t snapshotlen);
	virtual ~PacketIDMEFReporter();

	virtual void receive(Packet* p);

private:
	string analyzerId;  /**< analyzer id for IDMEF messages */
	string idmefTemplate;   /**< template file for IDMEF messages */
	uint16_t snapshotOffset;	/**< offset from start of payload where snapshot is taken */
	uint16_t snapshotLength;	/**< length of snapshot */
	char snapshotBuffer[3000];	/**< buffer for holding hexdump of snapshot */

	// manages instances of IDMEFMessages
	static InstanceManager<IDMEFMessage> idmefManager;
	
	void analyzePacket(Packet* p, IDMEFMessage* msg);
};

#endif /*PACKETIDMEFREPORTER_H_*/
